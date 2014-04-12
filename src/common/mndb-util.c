#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "mndb-util.h"

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
                   + format_len];

  size_t i = 0;
  strcpy(full_format + i, prefix); i += prefix_len;
  strcpy(full_format + i, tag); i += tag_len;
  strcpy(full_format + i, sep); i += sep_len;
  strcpy(full_format + i, log_levels[level]); i += level_len;
  strcpy(full_format + i, sep); i += sep_len;
  strcpy(full_format + i, format);

  va_start(args, format);
  vfprintf(stderr, full_format, args);
  va_end(args);
}
