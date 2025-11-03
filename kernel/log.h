// Logging library with file-specific log levels, file name, line number, file specific log enable/disable
#ifndef LOG_H
#define LOG_H
typedef enum {
  LOG_NONE  = 0,
  LOG_FATAL = 1,
  LOG_ERROR = 2,
  LOG_WARN  = 3,
  LOG_INFO  = 4,
  LOG_DEBUG = 5,
  LOG_TRACE = 6,
} log_level_t;
#endif

#if LOG_LEVEL==0 || LOG_ENABLE==0
static inline void log(log_level_t level, char *str, ...){
  (void) level;
  (void) str;
}
#else
#include "printf.h"
static inline const char *
log_level_color(log_level_t level)
{
  switch (level) {
  case LOG_TRACE: return "\x1b[00;34m";
  case LOG_DEBUG: return "\x1b[00;35m";
  case LOG_INFO:  return "\x1b[00;36m";
  case LOG_WARN:  return "\x1b[00;33m";
  case LOG_ERROR: return "\x1b[00;31m";
  case LOG_FATAL: return "\x1b[37;41m";
  case LOG_NONE:  return "\x1b[0m";
  }
  /* Should be unreachable */
  return 0;
}

static inline void log(log_level_t level, char *str, ...){
  va_list args;
  va_start(args, str);

  if(level >= LOG_LEVEL){
    printf(log_level_color(level));
    vprintf(str, args);
    printf(log_level_color(LOG_NONE));
  }

  va_end(args);
}
#endif

