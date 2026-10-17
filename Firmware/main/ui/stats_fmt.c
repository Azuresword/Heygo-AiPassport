// main/ui/stats_fmt.c
#include "stats_fmt.h"

#include <stdarg.h>
#include <stdio.h>

#define U8_UNKNOWN  0xFFu
#define U16_UNKNOWN 0xFFFFu

// Never leave a cut number on the screen: a text that does not fit gives "".
static size_t put(char *out, size_t cap, const char *fmt, ...) {
    if (!out || cap == 0) return 0;
    va_list ap;
    va_start(ap, fmt);
    const int n = vsnprintf(out, cap, fmt, ap);
    va_end(ap);
    if (n < 0 || (size_t)n >= cap) {
        out[0] = '\0';
        return 0;
    }
    return (size_t)n;
}

size_t stats_fmt_speed(char *out, size_t cap, uint16_t speed_dkmh) {
    if (speed_dkmh == U16_UNKNOWN) return put(out, cap, "%s", STATS_UNKNOWN);
    return put(out, cap, "%u", (unsigned)((speed_dkmh + 5u) / 10u));
}

size_t stats_fmt_drop(char *out, size_t cap, uint16_t drop_m) {
    if (drop_m == U16_UNKNOWN) return put(out, cap, "%s", STATS_UNKNOWN);
    return put(out, cap, "%u m", (unsigned)drop_m);
}

size_t stats_fmt_distance(char *out, size_t cap, uint16_t distance_m) {
    if (distance_m == U16_UNKNOWN) return put(out, cap, "%s", STATS_UNKNOWN);
    if (distance_m < 1000u) return put(out, cap, "%u m", (unsigned)distance_m);
    const unsigned tenths = ((unsigned)distance_m + 50u) / 100u;   // 0.1 km, rounded half up.
    return put(out, cap, "%u.%u km", tenths / 10u, tenths % 10u);
}

size_t stats_fmt_count(char *out, size_t cap, uint16_t count) {
    if (count == U16_UNKNOWN) return put(out, cap, "%s", STATS_UNKNOWN);
    return put(out, cap, "%u", (unsigned)count);
}

size_t stats_fmt_count8(char *out, size_t cap, uint8_t count) {
    if (count == U8_UNKNOWN) return put(out, cap, "%s", STATS_UNKNOWN);
    return put(out, cap, "%u", (unsigned)count);
}

size_t stats_fmt_time(char *out, size_t cap, uint16_t time_s) {
    if (time_s == U16_UNKNOWN) return put(out, cap, "%s", STATS_UNKNOWN);
    const unsigned s = time_s;
    if (s < 3600u) return put(out, cap, "%02u:%02u", s / 60u, s % 60u);
    return put(out, cap, "%u:%02u", s / 3600u, (s / 60u) % 60u);
}

size_t stats_fmt_iq(char *out, size_t cap, uint8_t iq) {
    if (iq > 100u) return put(out, cap, "%s", STATS_UNKNOWN);
    return put(out, cap, "%u", (unsigned)iq);
}
