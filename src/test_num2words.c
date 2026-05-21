#include "num2words.h"
#include <stdio.h>

#define BUFFER_SIZE 86

int main(void) {
    char buf[BUFFER_SIZE];

    for (int m = 0; m < 60; m++) {
        fuzzy_time_to_words(10, m, buf, BUFFER_SIZE);
        printf("10:%02d  %s\n", m, buf);
    }

    printf("\n--- noon ---\n");
    fuzzy_time_to_words(12, 0, buf, BUFFER_SIZE);
    printf("12:00  %s\n", buf);

    printf("\n--- midnight ---\n");
    fuzzy_time_to_words(0, 0, buf, BUFFER_SIZE);
    printf(" 0:00  %s\n", buf);

    return 0;
}
