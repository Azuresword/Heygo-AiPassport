// main/link/link_core.h
// The device side of the BLE protocol session: doc 07, protocol version 0.1.
//
// The core owns the handshake, the voice availability, the exchange IDs, and the
// periodic status packets. It does no I/O. It sends packets and reports events
// through the callbacks in link_io_t. Plain C, so host tests can link it.
//
// The caller serializes every call. The callbacks run inside the call that causes
// them, so a callback must not call back into the core.
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ble_proto.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LINK_HELLO_ACK_MS     2000    // Doc 07 section 8.
#define LINK_PHONE_SILENT_MS  15000   // Doc 07 section 8.
#define LINK_DEVICE_STATUS_MS 30000   // Doc 07 section 6.
#define LINK_ATT_OVERHEAD     3       // ATT header in each notification and write.
#define LINK_PACKET_MAX       244     // ATT MTU 247 minus the ATT header.
#define LINK_TEXT_MAX         240     // Longest AGENT_STATE text at MTU 247, plus room.

// Control packets go before audio packets. AGENT_END travels in the audio lane,
// so that it follows the last AUDIO_UP block. See doc 07 section 7, "Last block".
typedef enum { LINK_LANE_CONTROL = 0, LINK_LANE_AUDIO } link_lane_t;

// The reason why voice is not available. LINK_VOICE_OK means that it is available.
typedef enum {
    LINK_VOICE_OK = 0,
    LINK_VOICE_NO_LINK,      // No connection, or the phone does not subscribe to TX.
    LINK_VOICE_WAIT_HELLO,   // HELLO went out. HELLO_ACK has 2 s to arrive.
    LINK_VOICE_NO_ACK,       // No HELLO_ACK within 2 s.
    LINK_VOICE_VERSION,      // The phone uses another protocol version.
    LINK_VOICE_MTU,          // The ATT MTU is below 172.
    LINK_VOICE_SILENT,       // No packet from the phone for 15 s.
} link_voice_t;

typedef struct {
    void *ctx;
    void (*send)(void *ctx, link_lane_t lane, const uint8_t *pkt, size_t len);
    void (*drop_audio)(void *ctx);   // Drop the uplink audio that waits in the TX queue.
    void (*voice_changed)(void *ctx, link_voice_t voice);
    // The calls below come only for the current Agent exchange. Stale packets never reach them.
    void (*agent_state)(void *ctx, uint8_t state, uint8_t failure, const char *text);
    void (*play_start)(void *ctx);
    void (*audio_down)(void *ctx, const uint8_t *block, size_t len, uint8_t missing);
    void (*play_end)(void *ctx);
} link_io_t;

typedef struct {
    link_io_t io;
    const char *fw_version;
    bool connected;
    bool subscribed;
    bool hello_sent;
    bool acked;
    bool version_bad;
    uint16_t mtu;
    uint32_t hello_ms;
    uint32_t last_rx_ms;
    uint32_t status_ms;
    link_voice_t voice;
    bp_xid_gen_t xids;
    uint8_t agent_xid;     // Current Agent exchange. Zero: none.
    uint8_t up_seq;
    bool down_open;
    bp_seq_t down_seq;
    uint8_t battery;
    uint8_t mode;
    uint8_t page;
    uint16_t free_kib;
    uint32_t utc;          // UTC seconds from HELLO_ACK or PHONE_STATUS. Zero: unknown.
    uint32_t utc_ms;       // Local time when utc arrived.
} link_core_t;

void link_init(link_core_t *l, const link_io_t *io, const char *fw_version);

// Events from the BLE stack. now_ms is a free-running millisecond clock.
void link_on_connect(link_core_t *l, uint32_t now_ms);
void link_on_disconnect(link_core_t *l, uint32_t now_ms);
void link_on_mtu(link_core_t *l, uint16_t mtu, uint32_t now_ms);
void link_on_subscribe(link_core_t *l, bool on, uint32_t now_ms);
void link_on_rx(link_core_t *l, const uint8_t *buf, size_t len, uint32_t now_ms);
// Call about every 100 ms. It checks the timeouts and sends DEVICE_STATUS.
void link_tick(link_core_t *l, uint32_t now_ms);

link_voice_t link_voice(const link_core_t *l);

// Agent exchange, called by the app.
uint8_t link_agent_start(link_core_t *l);                      // Returns the xid, or 0 without voice.
void link_audio_up(link_core_t *l, const uint8_t block[BP_ADPCM_BLOCK]);
void link_agent_end(link_core_t *l, uint8_t reason);          // BP_END_RELEASE or BP_END_LIMIT.
void link_agent_cancel(link_core_t *l);
void link_agent_done(link_core_t *l);                         // The answer ended. Drop later packets.

// Device state for CONTEXT and DEVICE_STATUS. A change sends the packets at once.
void link_set_device(link_core_t *l, uint8_t battery, uint8_t mode, uint8_t page,
                     uint16_t free_kib, uint32_t now_ms);

const char *link_voice_name(link_voice_t voice);

#ifdef __cplusplus
}
#endif
