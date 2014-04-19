#include <stdlib.h>
#include <errno.h>

#include "mndb.h"

extern mndb_log_level_t _mndb_log_level;

bool
mndb_init(int *argc, char ***argv)
{
  char *log_level_str = getenv(MNDB_ENVVAR_DEBUG);
  if(log_level_str == NULL)
  {
    _mndb_log_level = MNDB_LOG_LEVEL_WARN;
  }
  else
  {
    char *end;
    long int log_level = strtol(log_level_str, &end, 10);
    if(end == log_level_str || *end != '\0' || errno == ERANGE)
    {
      _mndb_log_level = MNDB_LOG_LEVEL_WARN;
    }
    else
    {
      _mndb_log_level = (mndb_log_level_t) CLAMP(log_level, MNDB_LOG_LEVEL_DEBUG, MNDB_N_LOG_LEVELS - 1);
    }
  }

  return true;
}
