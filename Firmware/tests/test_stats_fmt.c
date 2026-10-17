// tests/test_stats_fmt.c
// Host test for main/ui/stats_fmt.c: the text of each Increment 2 number.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stats_fmt.h"

static int s_fail;
static char s_out[STATS_TEXT_MAX];

#define EXPECT(call, want) do {                                                         \
    size_t n_ = (call);                                                                 \
    if (strcmp(s_out, (want)) != 0 || n_ != strlen(want)) {                             \
        printf("FAIL %s:%d %s gave \"%s\", want \"%s\"\n", __FILE__, __LINE__, #call, s_out, (want)); \
        s_fail++;                                                                       \
    }                                                                                   \
} while (0)

#define O s_out, sizeof s_out

static void unknown_is_never_zero(void) {
    EXPECT(stats_fmt_speed(O, 0xFFFF), "--");
    EXPECT(stats_fmt_drop(O, 0xFFFF), "--");
    EXPECT(stats_fmt_distance(O, 0xFFFF), "--");
    EXPECT(stats_fmt_count(O, 0xFFFF), "--");
    EXPECT(stats_fmt_count8(O, 0xFF), "--");
    EXPECT(stats_fmt_time(O, 0xFFFF), "--");
    EXPECT(stats_fmt_iq(O, 0xFF), "--");
    EXPECT(stats_fmt_iq(O, 101), "--");
}

static void zero_stays_zero(void) {
    EXPECT(stats_fmt_speed(O, 0), "0");
    EXPECT(stats_fmt_drop(O, 0), "0 m");
    EXPECT(stats_fmt_distance(O, 0), "0 m");
    EXPECT(stats_fmt_count(O, 0), "0");
    EXPECT(stats_fmt_time(O, 0), "00:00");
    EXPECT(stats_fmt_iq(O, 0), "0");
}

static void values(void) {
    EXPECT(stats_fmt_speed(O, 383), "38");          // 38.3 km/h
    EXPECT(stats_fmt_speed(O, 385), "39");          // half up
    EXPECT(stats_fmt_speed(O, 65534), "6553");
    EXPECT(stats_fmt_drop(O, 312), "312 m");
    EXPECT(stats_fmt_distance(O, 999), "999 m");
    EXPECT(stats_fmt_distance(O, 1000), "1.0 km");
    EXPECT(stats_fmt_distance(O, 1849), "1.8 km");
    EXPECT(stats_fmt_distance(O, 1850), "1.9 km");  // half up
    EXPECT(stats_fmt_distance(O, 65534), "65.5 km");
    EXPECT(stats_fmt_count(O, 24), "24");
    EXPECT(stats_fmt_count8(O, 4), "4");
    EXPECT(stats_fmt_time(O, 151), "02:31");
    EXPECT(stats_fmt_time(O, 3599), "59:59");
    EXPECT(stats_fmt_time(O, 3600), "1:00");
    EXPECT(stats_fmt_time(O, 3725), "1:02");
    EXPECT(stats_fmt_time(O, 65534), "18:12");
    EXPECT(stats_fmt_iq(O, 82), "82");
    EXPECT(stats_fmt_iq(O, 100), "100");
}

static void small_buffer(void) {
    char tiny[4] = "xyz";
    if (stats_fmt_distance(tiny, sizeof tiny, 1850) != 0 || tiny[0] != '\0') {
        printf("FAIL a text that does not fit must give \"\", not a cut number\n");
        s_fail++;
    }
    if (stats_fmt_speed(NULL, 8, 100) != 0 || stats_fmt_speed(tiny, 0, 100) != 0) {
        printf("FAIL a missing buffer must give 0\n");
        s_fail++;
    }
}

int main(void) {
    unknown_is_never_zero();
    zero_stays_zero();
    values();
    small_buffer();
    if (s_fail) return EXIT_FAILURE;
    printf("stats format tests: PASS\n");
    return EXIT_SUCCESS;
}
