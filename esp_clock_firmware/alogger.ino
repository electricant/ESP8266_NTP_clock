/*
 * Arduino logger facility
 */
#include <stdarg.h>
#include "alogger.h"

#define CIRCULAR_LINE_INDEX(index) ((index) % NUM_LINES)

static char logbuf[NUM_LINES][LINE_LENGTH];
static size_t curLine = 0;

void logbuf_put(const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);

  curLine = CIRCULAR_LINE_INDEX(curLine + 1);
  vsnprintf(logbuf[curLine], LINE_LENGTH, fmt, args);
  
  va_end(args);
}

int logcat_cb(const char* param)
{
  // i starts from 1 to avoid printing curLine first
  for(size_t i = 1; i <= NUM_LINES; i++)
  {
    size_t index = CIRCULAR_LINE_INDEX(curLine + i);
    if (strlen(logbuf[index]) > 0)
      Serial.println(logbuf[index]);
   }
}
