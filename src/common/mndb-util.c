#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "common/mndb-util.h"

mndb_log_level_t _mndb_log_level = MNDB_LOG_LEVEL_WARN;

static const char *const log_levels[MNDB_N_LOG_LEVELS] = {
  "DEBUG",
  "INFO",
  "WARN",
  "ERROR",
  "FATAL",
  "UNKNOWN"
};

void
mndb_log(mndb_log_level_t level, const char *tag, const char *format, ...)
{
  if(level < _mndb_log_level) return;

  va_list args;

  const char *prefix = "mndb:";
  const char *sep1 = ":";
  const char *sep2 = ": ";

  size_t prefix_len = strlen(prefix);
  size_t level_len = strlen(log_levels[level]);
  size_t tag_len = strlen(tag);
  size_t sep1_len = strlen(sep1);
  size_t sep2_len = strlen(sep2);
  size_t format_len = strlen(format);

  char full_format[prefix_len
                   + level_len
                   + sep1_len
                   + tag_len
                   + sep2_len
                   + format_len
                   + 2];

  size_t i = 0;
  memcpy(full_format + i, prefix, prefix_len); i += prefix_len;
  memcpy(full_format + i, log_levels[level], level_len); i += level_len;
  memcpy(full_format + i, sep1, sep1_len); i += sep1_len;
  memcpy(full_format + i, tag, tag_len); i += tag_len;
  memcpy(full_format + i, sep2, sep2_len); i += sep2_len;
  memcpy(full_format + i, format, format_len); i += format_len;
  full_format[i] = '\n'; i++;
  full_format[i] = '\0'; i++;

  va_start(args, format);
  vfprintf(stderr, full_format, args);
  va_end(args);
}
