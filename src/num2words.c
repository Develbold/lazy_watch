#include "num2words.h"
#include "string.h"
#include "stdint.h"


#ifdef CAPITAL
static const char* const ONES[] = {
  "NULL",
  "EINS",
  "ZWEI",
  "DREI",
  "VIER",
  "FÜNF",
  "SECHS",
  "SIEBEN",
  "ACHT",
  "NEUN"
};
#elif defined HALF_CAPITAL
static const char* const ONES[] = {
  "Null",
  "Eins",
  "Zwei",
  "Drei",
  "Vier",
  "Fünf",
  "Sechs",
  "Sieben",
  "Acht",
  "Neun"
};
#else
static const char* const ONES[] = {
  "null",
  "eins",
  "zwei",
  "drei",
  "vier",
  "fünf",
  "sechs",
  "sieben",
  "acht",
  "neun"
};
#endif

#ifdef CAPITAL
static const char* const TEENS[] ={
  "",
  "ELF",
  "ZWÖLF",
  "DREIZEHN",
  "VIERZEHN",
  "FÜNFZEHN",
  "SECHZEHN",
  "SIEBZEHN",
  "ACHTZEHN",
  "NEUNZEHN"
};
#elif defined HALF_CAPITAL
static const char* const TEENS[] ={
  "",
  "Elf",
  "Zwölf",
  "Dreizehn",
  "Vierzehn",
  "Fünfzehn",
  "Sechzehn",
  "Siebzehn",
  "Achtzehn",
  "Neunzehn"
};
#else
static const char* const TEENS[] ={
  "",
  "elf",
  "zwölf",
  "dreizehn",
  "vierzehn",
  "fünfzehn",
  "sechzehn",
  "siebzehn",
  "achtzehn",
  "neunzehn"
};
#endif

#ifdef CAPITAL
static const char* const TENS[] = {
  "",
  "ZEHN",
  "ZWANZIG",
  "DREISSIG",
  "VIERZIG",
  "FÜNFZIG",
  "SECHZIG",
  "SIEBZIG",
  "ACHTZIG",
  "NEUNZIG"
};
#elif defined HALF_CAPITAL
static const char* const TENS[] = {
  "",
  "Zehn",
  "Zwanzig",
  "Dreissig",
  "Vierzig",
  "Fünfzig",
  "Sechzig",
  "Siebzig",
  "Achtzig",
  "Neunzig"
};
#else
static const char* const TENS[] = {
  "",
  "zehn",
  "zwanzig",
  "dreissig",
  "vierzig",
  "fünfzig",
  "sechzig",
  "siebzig",
  "achtzig",
  "neunzig"
};
#endif

#ifdef CAPITAL
static const char* STR_OH_CLOCK = "UHR";
static const char* STR_NOON = "ZWÖLF";
static const char* STR_MIDNIGHT = "NULL";
static const char* STR_QUARTER = "VIERTEL";
static const char* STR_TO = "VOR";
static const char* STR_HALF = "HALB";
static const char* STR_AFTER = "NACH";
static const char* STR_AND = "UND";
static const char* STR_EIN = "EIN";
#elif defined HALF_CAPITAL
static const char* STR_OH_CLOCK = "Uhr";
static const char* STR_NOON = "Zwölf";
static const char* STR_MIDNIGHT = "Null";
static const char* STR_QUARTER = "Viertel";
static const char* STR_TO = "Vor";
static const char* STR_HALF = "Halb";
static const char* STR_AFTER = "Nach";
static const char* STR_AND = "Und";
static const char* STR_EIN = "Ein";
#else
static const char* STR_OH_CLOCK = "uhr";
static const char* STR_NOON = "zwölf";
static const char* STR_MIDNIGHT = "null";
static const char* STR_QUARTER = "viertel";
static const char* STR_TO = "vor";
static const char* STR_HALF = "halb";
static const char* STR_AFTER = "nach";
static const char* STR_AND = "und";
static const char* STR_EIN = "ein";
#endif
static const char* STR_NEWLINE = "\n";

//this returns the correct pm/am hour,  instead of the %12 hour
static int get_cor_hour(uint8_t inc_hour)
{
  if (inc_hour < 12)
  {
    return inc_hour;
  }
  else if (inc_hour == 24)
  {
    return 0;
  }
  else
  {
    uint8_t val = (inc_hour %12);
    if (val == 0)
    {
      return 12;
    }
    else
    {
      return val;
    }
  }
}

static size_t append_string(char* buffer, const size_t length, const char* str) {
  strncat(buffer, str, length);

  size_t written = strlen(str);
  return (length > written) ? written : length;
}

static size_t append_number(char* words, size_t remaining, int num)
{
  int tens_val = num / 10 % 10;
  int ones_val = num % 10;

  size_t len = 0;
  //if there is only a "ones number", print it
  if (((ones_val > 0)&&(tens_val<1)) || num == 0)
  {
    len = append_string(words, remaining, ONES[ones_val]);
  }
  //if number is bigger then ten
  else if (tens_val > 0)
  {
    //if number is between 11 and 19
    if (tens_val == 1 && num != 10)
    {
      return append_string(words, remaining, TEENS[ones_val]);
    }
    //if number is a multiple of ten, print it
    else if (ones_val == 0)
    {
      len = append_string(words, remaining, TENS[tens_val]);
    }
    //if the number is between x1 and x9 and not below 20, print it
    else
    {
      //if the ones is a "eins" print a "ein"
      const char *first = (ones_val == 1) ? STR_EIN : ONES[ones_val];
      len += append_string(words, remaining, first);
      len += append_string(words, remaining - len, STR_NEWLINE);
      len += append_string(words, remaining - len, STR_AND);
      len += append_string(words, remaining - len, STR_NEWLINE);
      len += append_string(words, remaining - len, TENS[tens_val]);
    }
  }
  //return string length
  return len;
}

void fuzzy_time_to_words(int fuzzy_hours, int fuzzy_minutes, char* words, size_t length)
{
  size_t remaining = length;
  memset(words, 0, length);

  if (fuzzy_minutes != 0)
  {
    if (fuzzy_minutes == 5 || fuzzy_minutes == 10)
    {
      remaining -= append_number(words, remaining, fuzzy_minutes);
      remaining -= append_string(words, remaining, STR_NEWLINE);
      remaining -= append_string(words, remaining, STR_AFTER);
      remaining -= append_string(words, remaining, STR_NEWLINE);
      remaining -= append_number(words, remaining, get_cor_hour(fuzzy_hours));
    }
    //quarter past
    else if (fuzzy_minutes == 15)
     {
      remaining -= append_string(words, remaining, STR_QUARTER);
      remaining -= append_string(words, remaining, STR_NEWLINE);
      remaining -= append_string(words, remaining, STR_AFTER);
      remaining -= append_string(words, remaining, STR_NEWLINE);
      remaining -= append_number(words, remaining, get_cor_hour(fuzzy_hours));
     }
    //halb
    else if (fuzzy_minutes == 30)
    {
      remaining -= append_string(words, remaining, STR_HALF);
      remaining -= append_string(words, remaining, STR_NEWLINE);
      remaining -= append_number(words, remaining, get_cor_hour(fuzzy_hours + 1));
    }
    //quarter to
    else if (fuzzy_minutes == 45)
    {
      remaining -= append_string(words, remaining, STR_QUARTER);
      remaining -= append_string(words, remaining, STR_NEWLINE);
      remaining -= append_string(words, remaining, STR_TO);
      remaining -= append_string(words, remaining, STR_NEWLINE);
      fuzzy_hours = (fuzzy_hours + 1) % 24;
      remaining -= append_number(words, remaining, get_cor_hour(fuzzy_hours));
    }
    else if (fuzzy_minutes >= 50)
    {
      remaining -= append_number(words, remaining, 60 - fuzzy_minutes);
      remaining -= append_string(words, remaining, STR_NEWLINE);
      remaining -= append_string(words, remaining, STR_TO);
      remaining -= append_string(words, remaining, STR_NEWLINE);
      fuzzy_hours = (fuzzy_hours + 1) % 24;
      remaining -= append_number(words, remaining, get_cor_hour(fuzzy_hours));
    }
    else
    {
      int h = get_cor_hour(fuzzy_hours);
      remaining -= (h == 1)
          ? append_string(words, remaining, STR_EIN)
          : append_number(words, remaining, h);
      remaining -= append_string(words, remaining, STR_NEWLINE);
      remaining -= append_string(words, remaining, STR_OH_CLOCK);
      remaining -= append_string(words, remaining, STR_NEWLINE);
      remaining -= append_number(words, remaining, fuzzy_minutes);
    }
  }
  //midnight
  else if (fuzzy_hours == 0)
  {
    remaining -= append_string(words, remaining, STR_MIDNIGHT);
  }
  //noon
  else if (fuzzy_hours == 12)
  {
    remaining -= append_string(words, remaining, STR_NOON);
  }
  //hour flat
  else
  {
    remaining -= append_number(words, remaining, get_cor_hour(fuzzy_hours));
  }
}
