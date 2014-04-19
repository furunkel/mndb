#pragma once

#include "common/mndb-util.h"
#include "common/mndb-ygen.h"
#include "common/mndb-ptr-stack.h"

#define MNDB_ENVVAR_DEBUG "MNDB_DEBUG"

bool
mndb_init(int *argc, const char ***argv);

