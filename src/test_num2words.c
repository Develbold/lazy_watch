#include "num2words.h"
#include "night_mode.h"
#include <assert.h>
#include <stdio.h>

static void test_night_mode_is_active(void) {
    // same-day window, e.g. a lunchtime "night mode" from 12:00 to 14:00
    assert(!night_mode_is_active(11, 59, 12, 0, 14, 0));
    assert(night_mode_is_active(12, 0, 12, 0, 14, 0));
    assert(night_mode_is_active(13, 30, 12, 0, 14, 0));
    assert(!night_mode_is_active(14, 0, 12, 0, 14, 0));

    // overnight window, e.g. 22:00 to 06:00
    assert(night_mode_is_active(23, 0, 22, 0, 6, 0));
    assert(night_mode_is_active(0, 0, 22, 0, 6, 0));
    assert(night_mode_is_active(5, 59, 22, 0, 6, 0));
    assert(!night_mode_is_active(6, 0, 22, 0, 6, 0));
    assert(!night_mode_is_active(21, 59, 22, 0, 6, 0));

    // start == end covers the full day
    assert(night_mode_is_active(0, 0, 8, 30, 8, 30));
    assert(night_mode_is_active(23, 59, 8, 30, 8, 30));

    printf("test_night_mode_is_active: OK\n");
}

static void test_is_connector_word(void) {
    assert(fuzzy_time_is_connector_word("Vor"));
    assert(fuzzy_time_is_connector_word("Nach"));
    assert(fuzzy_time_is_connector_word("Uhr"));
    assert(!fuzzy_time_is_connector_word("Zehn"));
    assert(!fuzzy_time_is_connector_word("Viertel"));
    assert(!fuzzy_time_is_connector_word(""));
    assert(!fuzzy_time_is_connector_word("vor"));
    printf("test_is_connector_word: OK\n");
}

static void print_oneline(const char *buf) {
    for (const char *p = buf; *p; p++)
        putchar(*p == '\n' ? ' ' : *p);
}

static void print_time(int h, int m) {
    char buf[FUZZY_TIME_BUFFER_SIZE];
    fuzzy_time_to_words(h, m, buf, FUZZY_TIME_BUFFER_SIZE);
    printf("%2d:%02d  ", h, m);
    print_oneline(buf);
    putchar('\n');
}

static void print_hour(int h) {
    for (int m = 0; m < 60; m++)
        print_time(h, m);
}

int main(void) {
    test_is_connector_word();
    test_night_mode_is_active();

    print_hour(10);

    printf("\n--- hour 1 ---\n");
    print_hour(1);

    printf("\n--- noon ---\n");
    print_time(12, 0);

    printf("\n--- midnight ---\n");
    print_time(0, 0);

    return 0;
}
