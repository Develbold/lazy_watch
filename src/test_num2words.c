#include "num2words.h"
#include <assert.h>
#include <stdio.h>

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

    print_hour(10);

    printf("\n--- hour 1 ---\n");
    print_hour(1);

    printf("\n--- noon ---\n");
    print_time(12, 0);

    printf("\n--- midnight ---\n");
    print_time(0, 0);

    return 0;
}
