// main/link/ble_link.c
#include "ble_link.h"

#include <stdio.h>
#include <string.h>

#include "bsp_battery.h"
#include "demo_radio.h"
#include "esp_app_desc.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "host/ble_hs.h"
#include "host/ble_esp_gap.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

static const char *TAG = "ble_link";

#define CONTROL_QUEUE_DEPTH 8     // Doc 07 plans 4. HELLO_ACK alone makes 2 packets, so keep room.
#define AUDIO_QUEUE_DEPTH   8     // Doc 07 section 7: 8 audio packets, drop the oldest.
#define TICK_MS             100
#define BATTERY_MS          30000
#define TX_TASK_STACK       3072
#define TX_TASK_PRIO        6
#define ADV_INTERVAL_MS     100   // Doc 07 section 2.
#define NAME_PREFIX         "HeygoPP-"

// Doc 07 section 4. Units: interval 1.25 ms, supervision timeout 10 ms.
#define CONN_ITVL_MIN       6     // 7.5 ms
#define CONN_ITVL_MAX       12    // 15 ms
#define CONN_TIMEOUT        400   // 4 s
#define DATA_LEN_OCTETS     251
#define DATA_LEN_TIME_US    2120

// 6e400001-b5a3-f393-e0a9-e50e24dcca9e and the RX and TX characteristics, little-endian.
static const ble_uuid128_t NUS_SVC = BLE_UUID128_INIT(
    0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0, 0x93, 0xf3, 0xa3, 0xb5, 0x01, 0x00, 0x40, 0x6e);
static const ble_uuid128_t NUS_RX = BLE_UUID128_INIT(
    0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0, 0x93, 0xf3, 0xa3, 0xb5, 0x02, 0x00, 0x40, 0x6e);
static const ble_uuid128_t NUS_TX = BLE_UUID128_INIT(
    0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0, 0x93, 0xf3, 0xa3, 0xb5, 0x03, 0x00, 0x40, 0x6e);

typedef struct {
    uint16_t len;
    uint8_t data[LINK_PACKET_MAX];
} tx_item_t;

static ble_link_app_t s_app;
static link_core_t s_core;
static SemaphoreHandle_t s_mu;
static QueueHandle_t s_ctrl_q;
static QueueHandle_t s_audio_q;
static TaskHandle_t s_tx_task;
static esp_timer_handle_t s_tick;
static uint16_t s_tx_handle;
static volatile uint16_t s_conn = BLE_HS_CONN_HANDLE_NONE;
static uint8_t s_addr_type;
static char s_name[16];
static char s_fw[BP_VERSION_TEXT_MAX + 1];
static uint8_t s_mode = BP_MODE_SKI;
static uint8_t s_page = BP_PAGE_SKI;
static uint8_t s_battery = BP_BATTERY_UNKNOWN;
static uint32_t s_battery_ms;
static unsigned s_down_blocks;      // AUDIO_DOWN blocks of the current answer, for the log.
static unsigned s_down_missing;
static uint8_t s_drop_type, s_drop_xid;   // The last logged drop. One line for each stream.

static uint32_t now_ms(void) { return (uint32_t)(esp_timer_get_time() / 1000); }
static void lock(void) { xSemaphoreTake(s_mu, portMAX_DELAY); }
static void unlock(void) { xSemaphoreGive(s_mu); }

static uint16_t free_kib(void) {
    size_t kib = heap_caps_get_free_size(MALLOC_CAP_8BIT) / 1024;
    return kib > UINT16_MAX ? UINT16_MAX : (uint16_t)kib;
}

// ---------------------------------------------------------------- debug log
// One line for each packet, with its fields. dir is "tx" or "rx".
// AUDIO_UP, AUDIO_DOWN, and PHONE_STATUS are too frequent; the caller skips them.
static const char *agent_state_name(uint8_t st) {
    static const char *names[] = { "?", "accepted", "recognizing", "thinking", "answering", "done", "failed" };
    return st < sizeof names / sizeof names[0] ? names[st] : "?";
}

static void log_packet(const char *dir, const uint8_t *buf, size_t len) {
    bp_packet_t p;
    if (bp_parse(buf, len, &p) != BP_OK) {
        ESP_LOGW(TAG, "%s bad packet: %u bytes, type 0x%02x", dir, (unsigned)len, len ? buf[0] : 0);
        return;
    }
    const char *name = bp_type_name(p.type);
    switch (p.type) {
    case BP_HELLO: {
        bp_hello_t h;
        if (bp_read_hello(&p, &h) == BP_OK)
            ESP_LOGI(TAG, "%s HELLO %u.%u mask=%u mtu=%u fw=\"%.*s\"", dir, h.major, h.minor,
                     h.codec_mask, h.mtu, (int)h.fw.len, (const char *)h.fw.ptr);
        return;
    }
    case BP_HELLO_ACK: {
        bp_hello_ack_t a;
        if (bp_read_hello_ack(&p, &a) == BP_OK)
            ESP_LOGI(TAG, "%s HELLO_ACK %u.%u utc=%lu app=\"%.*s\"", dir, a.major, a.minor,
                     (unsigned long)a.utc, (int)a.app.len, (const char *)a.app.ptr);
        return;
    }
    case BP_ERROR: {
        bp_error_t e;
        if (bp_read_error(&p, &e) == BP_OK)
            ESP_LOGW(TAG, "%s ERROR code=%u ref=%s xid=0x%02x", dir, e.code, bp_type_name(e.ref_type), e.ref_xid);
        return;
    }
    case BP_CONTEXT: {
        bp_context_t c;
        if (bp_read_context(&p, &c) == BP_OK) ESP_LOGI(TAG, "%s CONTEXT mode=%u page=%u", dir, c.mode, c.page);
        return;
    }
    case BP_DEVICE_STATUS: {
        bp_device_status_t d;
        if (bp_read_device_status(&p, &d) == BP_OK)
            ESP_LOGI(TAG, "%s DEVICE_STATUS battery=%u mode=%u page=%u heap=%u KiB", dir, d.battery, d.mode, d.page, d.free_kib);
        return;
    }
    case BP_AGENT_START: {
        bp_agent_start_t a;
        if (bp_read_agent_start(&p, &a) == BP_OK)
            ESP_LOGI(TAG, "%s AGENT_START xid=0x%02x codec=%u rate=%u kHz", dir, p.xid, a.codec, a.rate_khz);
        return;
    }
    case BP_AGENT_END: {
        uint8_t reason = 0;
        if (bp_read_agent_end(&p, &reason) == BP_OK)
            ESP_LOGI(TAG, "%s AGENT_END xid=0x%02x reason=%s", dir, p.xid, reason == BP_END_LIMIT ? "20-second limit" : "release");
        return;
    }
    case BP_AGENT_STATE: {
        bp_agent_state_t a;
        if (bp_read_agent_state(&p, &a) == BP_OK)
            ESP_LOGI(TAG, "%s AGENT_STATE xid=0x%02x state=%s failure=%u text=\"%.*s\"", dir, p.xid,
                     agent_state_name(a.state), a.failure, (int)a.text.len, (const char *)a.text.ptr);
        return;
    }
    case BP_PLAY_START: {
        bp_play_start_t a;
        if (bp_read_play_start(&p, &a) == BP_OK)
            ESP_LOGI(TAG, "%s PLAY_START xid=0x%02x channel=%u codec=%u", dir, p.xid, a.channel, a.codec);
        return;
    }
    case BP_LIVE_STATS: {
        bp_live_stats_t s;
        if (bp_read_live_stats(&p, &s) == BP_OK)
            ESP_LOGI(TAG, "%s LIVE_STATS run=%u state=%u speed=%u drop=%u dist=%u turns=%u time=%u", dir,
                     s.run_no, s.run_state, s.speed_dkmh, s.drop_m, s.distance_m, s.turns, s.time_s);
        return;
    }
    case BP_RUN_REPORT: {
        bp_run_report_t r;
        if (bp_read_run_report(&p, &r) == BP_OK)
            ESP_LOGI(TAG, "%s RUN_REPORT run=%u dist=%u turns=%u iq=%u problem=\"%.*s\" tip=\"%.*s\"", dir,
                     r.run_no, r.distance_m, r.turns, r.iq, (int)r.problem.len, (const char *)r.problem.ptr,
                     (int)r.tip.len, (const char *)r.tip.ptr);
        return;
    }
    default:
        ESP_LOGI(TAG, "%s %s xid=0x%02x seq=%u len=%u", dir, name, p.xid, p.seq, (unsigned)p.payload_len);
        return;
    }
}

// ---------------------------------------------------------------- link_core I/O
static void io_send(void *ctx, link_lane_t lane, const uint8_t *pkt, size_t len) {
    (void)ctx;
    if (len && pkt[0] != BP_AUDIO_UP) log_packet("tx", pkt, len);
    tx_item_t item = { .len = (uint16_t)len };
    memcpy(item.data, pkt, len);
    if (lane == LINK_LANE_AUDIO) {
        // Doc 07: on overflow, drop the oldest audio packet.
        if (xQueueSend(s_audio_q, &item, 0) != pdTRUE) {
            tx_item_t old;
            (void)xQueueReceive(s_audio_q, &old, 0);
            (void)xQueueSend(s_audio_q, &item, 0);
        }
    } else if (xQueueSend(s_ctrl_q, &item, 0) != pdTRUE) {
        ESP_LOGW(TAG, "control queue full: drop type 0x%02x", pkt[0]);
    }
    xTaskNotifyGive(s_tx_task);
}

static void io_drop_audio(void *ctx) { (void)ctx; xQueueReset(s_audio_q); }

static void io_voice(void *ctx, link_voice_t v) {
    (void)ctx;
    ESP_LOGI(TAG, "voice: %s", link_voice_name(v));
    if (s_app.voice_changed) s_app.voice_changed(v);
}
static void io_state(void *ctx, uint8_t st, uint8_t f, const char *t) {
    (void)ctx;
    if (s_app.agent_state) s_app.agent_state(st, f, t);
}
static void io_play_start(void *ctx) {
    (void)ctx;
    s_down_blocks = 0;
    s_down_missing = 0;
    if (s_app.play_start) s_app.play_start();
}
static void io_down(void *ctx, const uint8_t *b, size_t n, uint8_t miss) {
    (void)ctx;
    s_down_blocks++;
    s_down_missing += miss;
    if (miss) ESP_LOGW(TAG, "AUDIO_DOWN gap: %u blocks before block %u", miss, s_down_blocks);
    if (s_app.audio_down) s_app.audio_down(b, n, miss);
}
static void io_play_end(void *ctx) {
    (void)ctx;
    ESP_LOGI(TAG, "answer received: %u AUDIO_DOWN blocks (%u ms), %u missing",
             s_down_blocks, s_down_blocks * 20, s_down_missing);
    if (s_app.play_end) s_app.play_end();
}
static void io_live(void *ctx, const bp_live_stats_t *s) {
    (void)ctx;
    if (s_app.live_stats) s_app.live_stats(s);
}
static void io_report(void *ctx, const link_report_t *r) {
    (void)ctx;
    if (s_app.run_report) s_app.run_report(r);
}
static void io_dropped(void *ctx, uint8_t type, uint8_t xid, uint8_t current) {
    (void)ctx;
    // AUDIO_DOWN of a stale stream comes 50 times a second. Log the first one only.
    if (type == s_drop_type && xid == s_drop_xid) return;
    s_drop_type = type;
    s_drop_xid = xid;
    ESP_LOGW(TAG, "drop %s xid=0x%02x: current exchange is 0x%02x (stale or canceled)",
             bp_type_name(type), xid, current);
}

// ---------------------------------------------------------------- TX task
// Control packets go first. One notification carries exactly one packet.
static void tx_task(void *arg) {
    (void)arg;
    tx_item_t item;
    for (;;) {
        bool have = xQueueReceive(s_ctrl_q, &item, 0) == pdTRUE ||
                    xQueueReceive(s_audio_q, &item, 0) == pdTRUE;
        if (!have) {
            (void)ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(50));
            continue;
        }
        const uint16_t conn = s_conn;
        if (conn == BLE_HS_CONN_HANDLE_NONE) continue;
        struct os_mbuf *om = NULL;
        for (int tries = 0; tries < 20 && !(om = ble_hs_mbuf_from_flat(item.data, item.len)); tries++) {
            vTaskDelay(pdMS_TO_TICKS(2));   // No free buffer yet. The stack frees one per sent packet.
        }
        if (!om) {
            ESP_LOGW(TAG, "no buffer: drop type 0x%02x", item.data[0]);
            continue;
        }
        int rc = ble_gatts_notify_custom(conn, s_tx_handle, om);   // The call takes om in every case.
        if (rc != 0) ESP_LOGW(TAG, "notify failed (%d): type 0x%02x", rc, item.data[0]);
    }
}

// ---------------------------------------------------------------- GATT
static int rx_access(uint16_t conn, uint16_t attr, struct ble_gatt_access_ctxt *ctxt, void *arg) {
    (void)conn; (void)attr; (void)arg;
    if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR) return BLE_ATT_ERR_UNLIKELY;
    uint8_t buf[LINK_PACKET_MAX];
    uint16_t len = 0;
    if (OS_MBUF_PKTLEN(ctxt->om) > sizeof buf) {
        // Doc 07: one write carries one packet, which fits the MTU. A longer write is wrong.
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }
    if (ble_hs_mbuf_to_flat(ctxt->om, buf, sizeof buf, &len) != 0) return BLE_ATT_ERR_UNLIKELY;
    if (len && buf[0] != BP_AUDIO_DOWN && buf[0] != BP_PHONE_STATUS) log_packet("rx", buf, len);
    lock();
    link_on_rx(&s_core, buf, len, now_ms());
    unlock();
    return 0;
}

static int tx_access(uint16_t conn, uint16_t attr, struct ble_gatt_access_ctxt *ctxt, void *arg) {
    (void)conn; (void)attr; (void)ctxt; (void)arg;
    return BLE_ATT_ERR_READ_NOT_PERMITTED;
}

static const struct ble_gatt_chr_def NUS_CHRS[] = {
    { .uuid = &NUS_RX.u, .access_cb = rx_access, .flags = BLE_GATT_CHR_F_WRITE_NO_RSP | BLE_GATT_CHR_F_WRITE },
    { .uuid = &NUS_TX.u, .access_cb = tx_access, .val_handle = &s_tx_handle, .flags = BLE_GATT_CHR_F_NOTIFY },
    { 0 },
};

static const struct ble_gatt_svc_def SVCS[] = {
    { .type = BLE_GATT_SVC_TYPE_PRIMARY, .uuid = &NUS_SVC.u, .characteristics = NUS_CHRS },
    { 0 },
};

// ---------------------------------------------------------------- GAP
static int gap_event(struct ble_gap_event *event, void *arg);

static void advertise(void) {
    struct ble_hs_adv_fields adv = { 0 };
    adv.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    adv.uuids128 = &NUS_SVC;
    adv.num_uuids128 = 1;
    adv.uuids128_is_complete = 1;
    int rc = ble_gap_adv_set_fields(&adv);

    struct ble_hs_adv_fields rsp = { 0 };
    rsp.name = (const uint8_t *)s_name;
    rsp.name_len = strlen(s_name);
    rsp.name_is_complete = 1;
    if (rc == 0) rc = ble_gap_adv_rsp_set_fields(&rsp);

    struct ble_gap_adv_params params = { 0 };
    params.conn_mode = BLE_GAP_CONN_MODE_UND;
    params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    params.itvl_min = BLE_GAP_ADV_ITVL_MS(ADV_INTERVAL_MS);
    params.itvl_max = BLE_GAP_ADV_ITVL_MS(ADV_INTERVAL_MS);
    if (rc == 0) rc = ble_gap_adv_start(s_addr_type, NULL, BLE_HS_FOREVER, &params, gap_event, NULL);
    if (rc != 0 && rc != BLE_HS_EALREADY) ESP_LOGE(TAG, "advertising failed: %d", rc);
    else ESP_LOGI(TAG, "advertising as %s", s_name);
}

// Doc 07 section 4: request the best-performance link once after the connection.
static void request_link_settings(uint16_t conn) {
    const struct ble_gap_upd_params p = {
        .itvl_min = CONN_ITVL_MIN, .itvl_max = CONN_ITVL_MAX, .latency = 0,
        .supervision_timeout = CONN_TIMEOUT, .min_ce_len = 0, .max_ce_len = 0,
    };
    int rc = ble_gap_update_params(conn, &p);
    if (rc) ESP_LOGW(TAG, "connection parameter request failed: %d", rc);
    rc = ble_gap_set_prefered_le_phy(conn, BLE_GAP_LE_PHY_2M_MASK | BLE_GAP_LE_PHY_1M_MASK,
                                     BLE_GAP_LE_PHY_2M_MASK | BLE_GAP_LE_PHY_1M_MASK, BLE_GAP_LE_PHY_CODED_ANY);
    if (rc) ESP_LOGW(TAG, "PHY request failed: %d", rc);
    rc = ble_hs_hci_util_set_data_len(conn, DATA_LEN_OCTETS, DATA_LEN_TIME_US);
    if (rc) ESP_LOGW(TAG, "data length request failed: %d", rc);
    // Doc 07 section 4: the phone requests the ATT MTU. The GATT client role is off in
    // sdkconfig, so the device cannot start the MTU exchange itself.
}

static void log_granted(uint16_t conn) {
    struct ble_gap_conn_desc d;
    if (ble_gap_conn_find(conn, &d) == 0) {
        // Doc 07: log the values that the phone grants.
        ESP_LOGI(TAG, "granted: interval %u x 1.25 ms, latency %u, timeout %u x 10 ms",
                 d.conn_itvl, d.conn_latency, d.supervision_timeout);
    }
}

static int gap_event(struct ble_gap_event *event, void *arg) {
    (void)arg;
    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status != 0) {
            advertise();
            break;
        }
        s_conn = event->connect.conn_handle;
        ESP_LOGI(TAG, "connected");
        lock();
        link_on_connect(&s_core, now_ms());
        unlock();
        request_link_settings(s_conn);
        break;
    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(TAG, "disconnected: reason 0x%x", event->disconnect.reason);
        s_conn = BLE_HS_CONN_HANDLE_NONE;
        lock();
        link_on_disconnect(&s_core, now_ms());
        unlock();
        xQueueReset(s_ctrl_q);
        xQueueReset(s_audio_q);
        advertise();
        break;
    case BLE_GAP_EVENT_MTU:
        ESP_LOGI(TAG, "MTU %u", event->mtu.value);
        lock();
        link_on_mtu(&s_core, event->mtu.value, now_ms());
        unlock();
        break;
    case BLE_GAP_EVENT_SUBSCRIBE:
        if (event->subscribe.attr_handle == s_tx_handle) {
            lock();
            link_on_subscribe(&s_core, event->subscribe.cur_notify, now_ms());
            unlock();
        }
        break;
    case BLE_GAP_EVENT_CONN_UPDATE:
        log_granted(event->conn_update.conn_handle);
        break;
    case BLE_GAP_EVENT_PHY_UPDATE_COMPLETE:
        ESP_LOGI(TAG, "PHY tx %u rx %u", event->phy_updated.tx_phy, event->phy_updated.rx_phy);
        break;
    case BLE_GAP_EVENT_ADV_COMPLETE:
        if (s_conn == BLE_HS_CONN_HANDLE_NONE) advertise();
        break;
    default:
        break;
    }
    return 0;
}

static void on_sync(void) {
    int rc = ble_hs_util_ensure_addr(0);
    if (rc == 0) rc = ble_hs_id_infer_auto(0, &s_addr_type);
    uint8_t addr[6] = { 0 };
    if (rc == 0) rc = ble_hs_id_copy_addr(s_addr_type, addr, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "address failed: %d", rc);
        return;
    }
    // Doc 07: XXXX holds the last two bytes of the address. NimBLE stores it little-endian.
    snprintf(s_name, sizeof s_name, NAME_PREFIX "%02X%02X", addr[1], addr[0]);
    ble_svc_gap_device_name_set(s_name);
    advertise();
}

static void on_reset(int reason) { ESP_LOGE(TAG, "NimBLE reset: %d", reason); }

static void host_task(void *arg) {
    (void)arg;
    nimble_port_run();
    nimble_port_freertos_deinit();
}

// ---------------------------------------------------------------- tick
static void on_tick(void *arg) {
    (void)arg;
    const uint32_t t = now_ms();
    if (s_battery_ms == 0 || (uint32_t)(t - s_battery_ms) >= BATTERY_MS) {
        const int soc = bsp_battery_soc();
        s_battery = (soc >= 0 && soc <= 100) ? (uint8_t)soc : BP_BATTERY_UNKNOWN;
        s_battery_ms = t ? t : 1;
    }
    lock();
    link_set_device(&s_core, s_battery, s_mode, s_page, free_kib(), t);
    link_tick(&s_core, t);
    unlock();
}

// ---------------------------------------------------------------- API
esp_err_t ble_link_start(const ble_link_app_t *app) {
    if (s_mu) return ESP_ERR_INVALID_STATE;
    if (app) s_app = *app;
    // Doc 07: the version text holds at most 32 bytes. "heygo " uses 6 of them.
    snprintf(s_fw, sizeof s_fw, "heygo %.26s", esp_app_get_description()->version);
    snprintf(s_name, sizeof s_name, NAME_PREFIX "0000");

    s_mu = xSemaphoreCreateMutex();
    s_ctrl_q = xQueueCreate(CONTROL_QUEUE_DEPTH, sizeof(tx_item_t));
    s_audio_q = xQueueCreate(AUDIO_QUEUE_DEPTH, sizeof(tx_item_t));
    if (!s_mu || !s_ctrl_q || !s_audio_q) return ESP_ERR_NO_MEM;
    if (xTaskCreate(tx_task, "ble_tx", TX_TASK_STACK, NULL, TX_TASK_PRIO, &s_tx_task) != pdPASS) {
        return ESP_ERR_NO_MEM;
    }

    const link_io_t io = { NULL, io_send, io_drop_audio, io_voice, io_state, io_play_start, io_down,
                           io_play_end, io_dropped, io_live, io_report };
    link_init(&s_core, &io, s_fw);

    esp_err_t err = demo_radio_nvs_prepare();
    if (err != ESP_OK) return err;
    // NimBLE logs 3 lines for each notification at INFO level: 150 lines a second during
    // capture. That fills about a third of the 115200-baud console and hides the app log.
    esp_log_level_set("NimBLE", ESP_LOG_WARN);
    err = nimble_port_init();
    if (err != ESP_OK) return err;

    ble_svc_gap_init();
    ble_svc_gatt_init();
    int rc = ble_gatts_count_cfg(SVCS);
    if (rc == 0) rc = ble_gatts_add_svcs(SVCS);
    if (rc != 0) {
        ESP_LOGE(TAG, "GATT service failed: %d", rc);
        return ESP_FAIL;
    }
    ble_hs_cfg.sync_cb = on_sync;
    ble_hs_cfg.reset_cb = on_reset;
    nimble_port_freertos_init(host_task);

    const esp_timer_create_args_t tick = { .callback = on_tick, .name = "ble_link" };
    err = esp_timer_create(&tick, &s_tick);
    if (err == ESP_OK) err = esp_timer_start_periodic(s_tick, TICK_MS * 1000);
    return err;
}

link_voice_t ble_link_voice(void) {
    if (!s_mu) return LINK_VOICE_NO_LINK;
    lock();
    const link_voice_t v = link_voice(&s_core);
    unlock();
    return v;
}

uint8_t ble_link_agent_start(void) {
    if (!s_mu) return 0;
    lock();
    const uint8_t xid = link_agent_start(&s_core);
    unlock();
    return xid;
}

void ble_link_audio_up(const uint8_t *block) {
    if (!s_mu) return;
    lock();
    link_audio_up(&s_core, block);
    unlock();
}

void ble_link_agent_end(uint8_t reason) {
    if (!s_mu) return;
    lock();
    link_agent_end(&s_core, reason);
    unlock();
}

void ble_link_agent_cancel(void) {
    if (!s_mu) return;
    lock();
    link_agent_cancel(&s_core);
    unlock();
}

void ble_link_agent_done(void) {
    if (!s_mu) return;
    lock();
    link_agent_done(&s_core);
    unlock();
}

void ble_link_get_data(bp_live_stats_t *live, bool *has_live, bool *stale,
                       link_report_t *report, bool *has_report, uint32_t *report_age_ms) {
    if (has_live) *has_live = false;
    if (has_report) *has_report = false;
    if (!s_mu) return;
    const uint32_t t = now_ms();
    bool old = true;
    uint32_t age = 0;
    lock();
    const bp_live_stats_t *l = link_live(&s_core, t, &old);
    const link_report_t *r = link_report(&s_core, t, &age);
    if (l && live) *live = *l;
    if (r && report) *report = *r;
    unlock();
    if (has_live) *has_live = l != NULL;
    if (stale) *stale = old;
    if (has_report) *has_report = r != NULL;
    if (report_age_ms) *report_age_ms = age;
}

void ble_link_set_context(uint8_t mode, uint8_t page) {
    if (!s_mu) return;
    lock();
    s_mode = mode;
    s_page = page;
    link_set_device(&s_core, s_battery, mode, page, free_kib(), now_ms());
    unlock();
}
