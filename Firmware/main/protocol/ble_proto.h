// main/protocol/ble_proto.h
// Packet layer for the BLE protocol draft: doc 07, protocol version 0.2.
// Plain C. No NimBLE, no ESP-IDF, and no LVGL, so host tests can link it.
//
// The first version builds the P0 messages and the Increment 2 data messages
// LIVE_STATS and RUN_REPORT. A P1 type (Team, PLAY_STOP) counts as an unknown
// type here, and gets error code 1.
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BP_PROTO_MAJOR     0
#define BP_PROTO_MINOR     2
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
    BP_LIVE_STATS    = 0x60,
    BP_RUN_REPORT    = 0x61,
    BP_SHOW_PAGE     = 0x62,
    BP_TURN_SHAPE    = 0x63,
    BP_TURN_GROUP    = 0x64,
    BP_TURN_REQUEST  = 0x65,
    BP_POPUP         = 0x66,
    BP_POPUP_END     = 0x67,
    BP_RUN_START     = 0x68,
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

// D-29 has no Ski or Rest mode. The wire values stay: mode 0 means the live data page or the
// Report page, and mode 1 means the Coach page. Page 2, the exit question, is no longer sent.
enum { BP_MODE_SKI = 0, BP_MODE_REST = 1 };
enum { BP_PAGE_SKI = 0, BP_PAGE_COACH = 1, BP_PAGE_EXIT = 2, BP_PAGE_REPORT = 3, BP_PAGE_TEAM = 4, BP_PAGE_TURNS = 5,
       BP_PAGE_POPUP = 6 };
enum { BP_CODEC_ADPCM = 0, BP_CODEC_PCM = 1 };
enum { BP_CODEC_MASK_ADPCM = 1u << BP_CODEC_ADPCM, BP_CODEC_MASK_PCM = 1u << BP_CODEC_PCM };
enum { BP_AGENT_ACCEPTED = 1, BP_AGENT_RECOGNIZING, BP_AGENT_THINKING,
       BP_AGENT_ANSWERING, BP_AGENT_DONE, BP_AGENT_FAILED };
enum { BP_FAIL_NONE = 0, BP_FAIL_RECOGNITION = 1, BP_FAIL_TIMEOUT = 2, BP_FAIL_GATEWAY = 3 };
enum { BP_END_RELEASE = 0, BP_END_LIMIT = 1 };
enum { BP_CHANNEL_AGENT = 0, BP_CHANNEL_TEAM = 1, BP_CHANNEL_POPUP = 2 };
#define BP_BATTERY_UNKNOWN 0xFF
// Increment 2 data, doc 07 section 6. The phone sends these values when it cannot
// calculate a field. The device shows them as "--" and never as 0.
#define BP_U8_UNKNOWN      0xFF
#define BP_U16_UNKNOWN     0xFFFF
enum { BP_RUN_UNKNOWN = 0, BP_RUN_SKIING = 1, BP_RUN_STOPPED = 2, BP_RUN_ENDED = 3 };
#define BP_LIVE_STATS_SIZE   12
#define BP_RUN_REPORT_FIXED  7
#define BP_IQ_MAX            200    // D-27: the Heygo IQ runs from 0 to 200.
#define BP_PROBLEM_MAX       36     // 12 Chinese characters in UTF-8.
#define BP_TIP_MAX           60     // 20 Chinese characters in UTF-8.
#define BP_FLAG_LAST       0x01
// TURN_SHAPE, TURN_GROUP, and TURN_REQUEST, doc 07 section 6. TURN_SHAPE gives the turn count of
// a run after RUN_REPORT. The device asks for groups of 8 turns in TURN_REQUEST. The phone sends
// each group in TURN_GROUP. Each group has its own points, from 0 to 255 on both axes.
#define BP_TURN_SHAPE_SIZE    3
#define BP_TURN_GROUP_FIXED   4
#define BP_TURN_ENTRY         6
#define BP_TURN_REQUEST_SIZE  3
// D-31: a popup from the phone. Doc 07, "Popups".
enum { BP_POPUP_FALL = 1, BP_POPUP_VOICE = 2, BP_POPUP_ISSUE = 3, BP_POPUP_BADGE = 4 };
// Why the popup closed. A lost link sends nothing: a new connection resets every xid.
enum { BP_POPUP_END_TIMEOUT = 0, BP_POPUP_END_KEY = 1, BP_POPUP_END_REPLACED = 2 };
#define BP_POPUP_FIXED        3
#define BP_POPUP_TITLE_MAX    36     // 12 Chinese characters in UTF-8.
#define BP_POPUP_BODY_MAX     120    // 40 Chinese characters in UTF-8.
#define BP_TURNS_MAX          255
#define BP_GROUP_TURNS        8
#define BP_GROUP_POINTS_MIN   2
#define BP_GROUP_POINTS_MAX   33     // 4 segments for each of 8 turns, and the first point.
#define BP_REQUEST_GROUPS_MAX 3
#define BP_EDGE_MAX          90     // Degrees.
enum { BP_TURN_SRC_MEASURED = 0, BP_TURN_SRC_SCHEMATIC = 1 };
enum { BP_SIDE_TOE = 0, BP_SIDE_HEEL = 1 };   // BP_U8_UNKNOWN: unknown side.

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
typedef struct {
    uint8_t run_state;       // BP_RUN_*. An unknown value reads as BP_RUN_UNKNOWN.
    uint8_t run_no;          // Run number today, from 1.
    uint16_t speed_dkmh;     // 0.1 km/h.
    uint16_t drop_m;
    uint16_t distance_m;
    uint16_t turns;
    uint16_t time_s;
} bp_live_stats_t;
typedef struct {
    uint8_t run_no;
    uint16_t distance_m;
    uint16_t turns;
    uint8_t iq;              // 0 to 100, or BP_U8_UNKNOWN.
    bp_text_t problem;       // At most BP_PROBLEM_MAX bytes of UTF-8.
    bp_text_t tip;           // At most BP_TIP_MAX bytes of UTF-8.
} bp_run_report_t;
typedef struct {
    uint8_t first, last;     // Point indexes in its group, first < last < point count of the group.
    uint8_t side;            // BP_SIDE_TOE, BP_SIDE_HEEL, or BP_U8_UNKNOWN.
    uint8_t speed_kmh;       // Each metric: BP_U8_UNKNOWN when the phone has no value.
    uint8_t edge_deg;        // 0 to BP_EDGE_MAX.
    uint8_t radius_m;
} bp_turn_t;
typedef struct {
    uint8_t run_no;
    uint8_t source;          // BP_TURN_SRC_*.
    uint8_t n_turns;         // The turn count of the whole run, 0 to 255.
} bp_turn_shape_t;
typedef struct {
    uint8_t run_no;
    uint8_t group;           // Group g holds turns 8g to 8g + 7 of the run.
    uint8_t n_turns;         // 1 to 8.
    uint8_t n_points;        // 2 to 33.
    bp_turn_t turns[BP_GROUP_TURNS];
    const uint8_t *points;   // n_points pairs of x, y, from 0 to 255. y grows downhill.
} bp_turn_group_t;
typedef struct { uint8_t run_no, first_group, count; } bp_turn_request_t;

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
bp_err_t bp_read_live_stats(const bp_packet_t *p, bp_live_stats_t *out);
// Gives BP_ERR_TOO_LARGE for a long text and BP_ERR_FIELD for bad UTF-8 or an IQ above 100.
bp_err_t bp_read_run_report(const bp_packet_t *p, bp_run_report_t *out);
// The phone asks the device to show a page. Only BP_PAGE_REPORT is valid. Another page gives BP_ERR_FIELD.
bp_err_t bp_read_show_page(const bp_packet_t *p, uint8_t *page);
// Gives BP_ERR_FIELD for a source above 1.
bp_err_t bp_read_turn_shape(const bp_packet_t *p, bp_turn_shape_t *out);
// Errors: BP_ERR_LENGTH when the length does not match the counts, BP_ERR_TOO_LARGE above
// 8 turns or 33 points, BP_ERR_FIELD for any other value out of range.
// The link session checks the run number and the group against the last TURN_SHAPE.
bp_err_t bp_read_turn_group(const bp_packet_t *p, bp_turn_group_t *out);
// Gives BP_ERR_FIELD for a count of 0 or above 3.
bp_err_t bp_read_turn_request(const bp_packet_t *p, bp_turn_request_t *out);
typedef struct {
    uint8_t kind;            // BP_POPUP_*.
    bool audio;              // true: a PLAY_START on channel 2 with the same xid follows.
    bp_text_t title;
    bp_text_t body;
} bp_popup_t;
// Gives BP_ERR_FIELD for an unknown kind, a bad audio flag, or bad UTF-8, and BP_ERR_TOO_LARGE for a long text.
bp_err_t bp_read_popup(const bp_packet_t *p, bp_popup_t *out);
bp_err_t bp_read_popup_end(const bp_packet_t *p, uint8_t *reason);
// D-32: the phone starts a run. Gives BP_ERR_FIELD for run number 0 or 0xFF.
bp_err_t bp_read_run_start(const bp_packet_t *p, uint8_t *run_no);

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
size_t bp_build_live_stats(uint8_t *out, size_t cap, const bp_live_stats_t *s);
size_t bp_build_run_report(uint8_t *out, size_t cap, uint8_t run_no, uint16_t distance_m, uint16_t turns,
                           uint8_t iq, const char *problem, const char *tip);
size_t bp_build_show_page(uint8_t *out, size_t cap, uint8_t page);
// Each turn builder returns 0 for a value that its reader rejects.
size_t bp_build_turn_shape(uint8_t *out, size_t cap, uint8_t run_no, uint8_t source, uint8_t n_turns);
// points holds n_points pairs of x, y.
size_t bp_build_turn_group(uint8_t *out, size_t cap, uint8_t run_no, uint8_t group, uint8_t n_turns,
                           const bp_turn_t *turns, uint8_t n_points, const uint8_t *points);
size_t bp_build_turn_request(uint8_t *out, size_t cap, uint8_t run_no, uint8_t first_group, uint8_t count);
size_t bp_build_popup(uint8_t *out, size_t cap, uint8_t xid, uint8_t kind, bool audio, const char *title, const char *body);
size_t bp_build_popup_end(uint8_t *out, size_t cap, uint8_t xid, uint8_t reason);
size_t bp_build_run_start(uint8_t *out, size_t cap, uint8_t run_no);

// The name of a type, for logs. Returns "?" for an unknown type.
const char *bp_type_name(uint8_t type);

// While the major version is 0, both sides must use the same major and minor.
bp_err_t bp_check_version(uint8_t major, uint8_t minor);
// True when the bytes are complete UTF-8 with no overlong form.
bool bp_utf8_valid(const uint8_t *p, size_t len);
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
