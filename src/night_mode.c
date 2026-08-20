#include "night_mode.h"

bool night_mode_is_active(int hour, int minute,
                           int start_hour, int start_minute,
                           int end_hour, int end_minute)
{
  int current = hour * 60 + minute;
  int start = start_hour * 60 + start_minute;
  int end = end_hour * 60 + end_minute;

  if (start == end) return true;
  if (start < end) return current >= start && current < end;
  return current >= start || current < end;
}
