#pragma once

#include "common/mndb-util.h"
#include "common/mndb-ygen.h"
#include "common/mndb-ptr-stack.h"
#include "common/mndb-str-buf.h"

#include "parser/mndb-parser.h"

#define MNDB_ENVVAR_LOG "MNDB_LOG"

bool
mndb_init(int *argc, const char ***argv);

