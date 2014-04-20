#pragma once

#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define CLAMP(x, min, max) (((x) > (max)) ? (max) : (((x) < (min)) ? (min) : (x)))

typedef enum {
  MNDB_LOG_LEVEL_DEBUG,
  MNDB_LOG_LEVEL_INFO,
  MNDB_LOG_LEVEL_WARN,
  MNDB_LOG_LEVEL_ERROR,
  MNDB_LOG_LEVEL_FATAL,
  MNDB_LOG_LEVEL_UNKNOWN,
  MNDB_N_LOG_LEVELS
} mndb_log_level_t;

#define MNDB_ALIGN(s, align) (((s) + align - 1) &~ (align - 1))
#define MNDB_ARY_LEN(ary) (sizeof(ary) / sizeof(ary[0]))

void
mndb_log(mndb_log_level_t level, const char *tag, const char *format, ...) __attribute__ ((format(printf, 3, 4)));

#ifndef NDEBUG
  #define mndb_debug(format, ...) mndb_log(MNDB_LOG_LEVEL_DEBUG, _mndb_log_tag, format, ##__VA_ARGS__)
#else
  #define mndb_debug(format, ...)
#endif

#define mndb_info(format, ...)  mndb_log(MNDB_LOG_LEVEL_INFO, _mndb_log_tag, format, ##__VA_ARGS__)
#define mndb_warn(format, ...)  mndb_log(MNDB_LOG_LEVEL_WARN, _mndb_log_tag, format, ##__VA_ARGS__)
#define mndb_error(format, ...) mndb_log(MNDB_LOG_LEVEL_ERROR, _mndb_log_tag, format, ##__VA_ARGS__)
#define mndb_fatal(format, ...) mndb_log(MNDB_LOG_LEVEL_FATAL, _mndb_log_tag, format, ##__VA_ARGS__)

#define unlikely(e) (__builtin_expect(e, 0))
#define likely(e) (__builtin_expect(e, 0))
