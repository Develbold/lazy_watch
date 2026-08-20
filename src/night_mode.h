#pragma once

#include <stdbool.h>

/* true if the given time-of-day falls within [start, end), handling the
 * overnight case where end is earlier than start (e.g. 22:00-06:00). If
 * start equals end, the window covers the full day. */
bool night_mode_is_active(int hour, int minute,
                           int start_hour, int start_minute,
                           int end_hour, int end_minute);
