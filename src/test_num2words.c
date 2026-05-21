#include "num2words.h"
#include <stdio.h>

#define BUFFER_SIZE 86

static void print_oneline(const char *buf) {
    for (const char *p = buf; *p; p++)
        putchar(*p == '\n' ? ' ' : *p);
}

int main(void) {
    char buf[BUFFER_SIZE];

    for (int m = 0; m < 60; m++) {
        fuzzy_time_to_words(10, m, buf, BUFFER_SIZE);
        printf("10:%02d  ", m);
        print_oneline(buf);
        putchar('\n');
    }

    printf("\n--- noon ---\n");
    fuzzy_time_to_words(12, 0, buf, BUFFER_SIZE);
    printf("12:00  ");
    print_oneline(buf);
    putchar('\n');

    printf("\n--- midnight ---\n");
    fuzzy_time_to_words(0, 0, buf, BUFFER_SIZE);
    printf(" 0:00  ");
    print_oneline(buf);
    putchar('\n');

    return 0;
}
