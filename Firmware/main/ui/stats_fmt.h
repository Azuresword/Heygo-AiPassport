// main/ui/stats_fmt.h
// Text for the Increment 2 numbers: doc 07 section 6 and requirements section 7.
// Plain C, so host tests can link it.
//
// Each function writes a text that ends with a zero byte, and returns its length.
// An unknown value (BP_U8_UNKNOWN or BP_U16_UNKNOWN) gives STATS_UNKNOWN and never "0".
// A buffer of STATS_TEXT_MAX bytes holds every result.
#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define STATS_UNKNOWN  "--"
#define STATS_TEXT_MAX 16

// 0.1 km/h to whole km/h, rounded half up. No unit: "38".
size_t stats_fmt_speed(char *out, size_t cap, uint16_t speed_dkmh);
// Meters with the unit: "312 m".
size_t stats_fmt_drop(char *out, size_t cap, uint16_t drop_m);
// Below 1,000 m in meters ("850 m"). From 1,000 m in km with one decimal ("1.8 km").
size_t stats_fmt_distance(char *out, size_t cap, uint16_t distance_m);
// A count with no unit: "24". Also for the run number.
size_t stats_fmt_count(char *out, size_t cap, uint16_t count);
size_t stats_fmt_count8(char *out, size_t cap, uint8_t count);
// Below 1 hour as min:s ("02:31"). From 1 hour as h:min ("1:02").
size_t stats_fmt_time(char *out, size_t cap, uint16_t time_s);
// Heygo IQ, 0 to 100: "82". A value above 100 also gives STATS_UNKNOWN.
size_t stats_fmt_iq(char *out, size_t cap, uint8_t iq);

#ifdef __cplusplus
}
#endif
