#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "common/mndb-util.h"

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
  va_list args;

  const char *prefix = "mndb: ";
  const char *sep = ":";

  size_t prefix_len = strlen(prefix);
  size_t tag_len = strlen(tag);
  size_t level_len = strlen(log_levels[level]);
  size_t sep_len = strlen(sep);
  size_t format_len = strlen(format);

  char full_format[prefix_len
                   + tag_len
                   + sep_len
                   + level_len
                   + sep_len
                   + format_len
                   + 2];

  size_t i = 0;
  memcpy(full_format + i, prefix, prefix_len); i += prefix_len;
  memcpy(full_format + i, tag, tag_len); i += tag_len;
  memcpy(full_format + i, sep, sep_len); i += sep_len;
  memcpy(full_format + i, log_levels[level], level_len); i += level_len;
  memcpy(full_format + i, sep, sep_len); i += sep_len;
  memcpy(full_format + i, format, format_len); i += format_len;
  full_format[i] = '\n'; i++;
  full_format[i] = '\0'; i++;

  va_start(args, format);
  vfprintf(stderr, full_format, args);
  va_end(args);
}
