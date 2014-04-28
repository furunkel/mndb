#pragma once

#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

typedef struct {
  size_t len;
  size_t capa;

  char *data;
} mndb_str_buf_t;


void
mndb_str_buf_init(mndb_str_buf_t *buf, size_t capa);

bool
mndb_str_buf_append(mndb_str_buf_t *buf, const char *data, ssize_t len);

bool
mndb_str_buf_appendf(mndb_str_buf_t *buf, const char *format, ...) __attribute__ ((format(printf, 2, 3)));

void
mndb_str_buf_destroy(mndb_str_buf_t *buf);

char *
mndb_str_buf_str(mndb_str_buf_t *buf, size_t *len);

