// main/protocol/ble_proto.h
// Packet layer for the BLE protocol draft: doc 07, protocol version 0.1.
// Plain C. No NimBLE, no ESP-IDF, and no LVGL, so host tests can link it.
//
// The first version builds only the P0 messages. A P1 type (Team, Reports,
// PLAY_STOP) counts as an unknown type here, and gets error code 1.
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BP_PROTO_MAJOR     0
#define BP_PROTO_MINOR     1
#define BP_HEADER_SIZE     4
#define BP_MTU_MIN         172
#define BP_ADPCM_BLOCK     164      // 20 ms at 16 kHz, doc 07 section 7
#define BP_PCM_BLOCK       160      // 5 ms at 16 kHz, 16-bit
#define BP_VERSION_TEXT_MAX 32
#define BP_SAMPLE_RATE_KHZ 16

typedef enum {
    BP_HELLO         = 0x01,
    BP_HELLO_ACK     = 0x02,
    BP_PHONE_STATUS  = 0x03,
    BP_DEVICE_STATUS = 0x04,
    BP_ERROR         = 0x0F,
    BP_CONTEXT       = 0x10,
    BP_AGENT_START   = 0x20,
    BP_AGENT_END     = 0x21,
    BP_AGENT_CANCEL  = 0x22,
    BP_AGENT_STATE   = 0x23,
    BP_AUDIO_UP      = 0x30,
    BP_AUDIO_DOWN    = 0x31,
    BP_PLAY_START    = 0x32,
    BP_PLAY_END      = 0x33,
} bp_type_t;

typedef enum {
    BP_OK               = 0,
    BP_ERR_UNKNOWN_TYPE = 1,
    BP_ERR_LENGTH       = 2,
    BP_ERR_VERSION      = 3,
    BP_ERR_TOO_LARGE    = 4,
    BP_ERR_SEQ_GAP      = 5,
    BP_ERR_STATE        = 6,
    BP_ERR_FIELD        = 7,
} bp_err_t;

enum { BP_MODE_SKI = 0, BP_MODE_REST = 1 };
enum { BP_PAGE_SKI = 0, BP_PAGE_COACH = 1, BP_PAGE_EXIT = 2, BP_PAGE_REPORTS = 3, BP_PAGE_TEAM = 4 };
enum { BP_CODEC_ADPCM = 0, BP_CODEC_PCM = 1 };
enum { BP_CODEC_MASK_ADPCM = 1u << BP_CODEC_ADPCM, BP_CODEC_MASK_PCM = 1u << BP_CODEC_PCM };
enum { BP_AGENT_ACCEPTED = 1, BP_AGENT_RECOGNIZING, BP_AGENT_THINKING,
       BP_AGENT_ANSWERING, BP_AGENT_DONE, BP_AGENT_FAILED };
enum { BP_FAIL_NONE = 0, BP_FAIL_RECOGNITION = 1, BP_FAIL_TIMEOUT = 2, BP_FAIL_GATEWAY = 3 };
enum { BP_END_RELEASE = 0, BP_END_LIMIT = 1 };
enum { BP_CHANNEL_AGENT = 0, BP_CHANNEL_TEAM = 1 };
#define BP_BATTERY_UNKNOWN 0xFF
#define BP_FLAG_LAST       0x01

typedef struct {
    uint8_t type, flags, xid, seq;
    const uint8_t *payload;
    size_t payload_len;
} bp_packet_t;

typedef struct { const uint8_t *ptr; size_t len; } bp_text_t;

typedef struct { uint8_t major, minor, codec_mask; uint16_t mtu; bp_text_t fw; } bp_hello_t;
typedef struct { uint8_t major, minor; uint32_t utc; bp_text_t app; } bp_hello_ack_t;
typedef struct { uint32_t utc; bool online, gateway, background; } bp_phone_status_t;
typedef struct { uint8_t battery, mode, page; uint16_t free_kib; } bp_device_status_t;
typedef struct { uint8_t code, ref_type, ref_xid; } bp_error_t;
typedef struct { uint8_t mode, page; } bp_context_t;
typedef struct { uint8_t codec, rate_khz; } bp_agent_start_t;
typedef struct { uint8_t state, failure; bp_text_t text; } bp_agent_state_t;
typedef struct { uint8_t channel, codec; bp_text_t speaker; } bp_play_start_t;

// Parse the header and check the minimum payload length of a P0 type.
// Extra bytes at the end of a fixed payload stay allowed.
bp_err_t bp_parse(const uint8_t *buf, size_t len, bp_packet_t *out);

bp_err_t bp_read_hello(const bp_packet_t *p, bp_hello_t *out);
bp_err_t bp_read_hello_ack(const bp_packet_t *p, bp_hello_ack_t *out);
bp_err_t bp_read_phone_status(const bp_packet_t *p, bp_phone_status_t *out);
bp_err_t bp_read_device_status(const bp_packet_t *p, bp_device_status_t *out);
bp_err_t bp_read_error(const bp_packet_t *p, bp_error_t *out);
bp_err_t bp_read_context(const bp_packet_t *p, bp_context_t *out);
bp_err_t bp_read_agent_start(const bp_packet_t *p, bp_agent_start_t *out);
bp_err_t bp_read_agent_end(const bp_packet_t *p, uint8_t *reason);
bp_err_t bp_read_agent_state(const bp_packet_t *p, bp_agent_state_t *out);
bp_err_t bp_read_play_start(const bp_packet_t *p, bp_play_start_t *out);

// Each builder returns the packet length, or 0 when `cap` is too small or a value is invalid.
size_t bp_build_hello(uint8_t *out, size_t cap, uint8_t codec_mask, uint16_t mtu, const char *fw);
size_t bp_build_hello_ack(uint8_t *out, size_t cap, uint8_t major, uint8_t minor, uint32_t utc, const char *app);
size_t bp_build_phone_status(uint8_t *out, size_t cap, uint32_t utc, bool online, bool gateway, bool background);
size_t bp_build_device_status(uint8_t *out, size_t cap, uint8_t battery, uint8_t mode, uint8_t page, uint16_t free_kib);
size_t bp_build_error(uint8_t *out, size_t cap, uint8_t code, uint8_t ref_type, uint8_t ref_xid);
size_t bp_build_context(uint8_t *out, size_t cap, uint8_t mode, uint8_t page);
size_t bp_build_agent_start(uint8_t *out, size_t cap, uint8_t xid, uint8_t codec);
size_t bp_build_agent_end(uint8_t *out, size_t cap, uint8_t xid, uint8_t reason);
size_t bp_build_agent_cancel(uint8_t *out, size_t cap, uint8_t xid);
size_t bp_build_agent_state(uint8_t *out, size_t cap, uint8_t xid, uint8_t state, uint8_t failure, const char *text);
size_t bp_build_audio(uint8_t *out, size_t cap, uint8_t type, uint8_t xid, uint8_t seq,
                      const uint8_t *block, size_t block_len);
size_t bp_build_play_start(uint8_t *out, size_t cap, uint8_t xid, uint8_t channel, uint8_t codec, const char *speaker);
size_t bp_build_play_end(uint8_t *out, size_t cap, uint8_t xid);

// The name of a type, for logs. Returns "?" for an unknown type.
const char *bp_type_name(uint8_t type);

// While the major version is 0, both sides must use the same major and minor.
bp_err_t bp_check_version(uint8_t major, uint8_t minor);
// Returns 0 for an unknown codec.
size_t bp_audio_block_size(uint8_t codec);
bp_err_t bp_check_audio_block(uint8_t codec, size_t len);

// Exchange IDs. The device uses 0x01 to 0x7F. The phone uses 0x80 to 0xFE.
typedef struct { uint8_t next, lo, hi; } bp_xid_gen_t;
void bp_xid_init_device(bp_xid_gen_t *g);
void bp_xid_init_phone(bp_xid_gen_t *g);
uint8_t bp_xid_next(bp_xid_gen_t *g);
// The device keeps one current xid per audio owner. Zero means that no
// exchange is open, so every packet gets dropped.
bool bp_xid_accept(uint8_t current, uint8_t xid);

// Audio sequence numbers. The first block of a stream has seq 0.
typedef struct { uint8_t expect; bool started; } bp_seq_t;
void bp_seq_reset(bp_seq_t *s);
// Returns the number of blocks missing before this one. Zero means in order.
uint8_t bp_seq_accept(bp_seq_t *s, uint8_t seq);
