#pragma once

#include "string.h"
#include <stdbool.h>
/*
typedef enum
{
  FALSE,
  TRUE
} boolean;
*/

/* the letter type can be defined with the following #defines:
 * #define CAPITAL: all letters are in CAPITAL
 * #define HALF_CAPITAL: only the first letter of a word is in Capital
 * "nothing defines": all letters are small                                  */
//#define CAPITAL
#define HALF_CAPITAL

#define FUZZY_TIME_BUFFER_SIZE 86

void fuzzy_time_to_words(int hours, int minutes, char* words, size_t length);

/* true if `line` is exactly one of the connector words ("vor"/"nach"/"uhr",
 * cased per the CAPITAL/HALF_CAPITAL setting above) rather than a number */
bool fuzzy_time_is_connector_word(const char* line);
