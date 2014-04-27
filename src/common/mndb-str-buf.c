#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>

#include "common/mndb-str-buf.h"
#include "common/mndb-util.h"

void
mndb_str_buf_init(mndb_str_buf_t *buf, size_t capa)
{
  buf->data  = malloc(capa * sizeof(char));
  buf->capa = capa;
  buf->len = 0;
}

static bool
mndb_str_buf_resize(mndb_str_buf_t *buf, size_t new_capa)
{
  char *new_data = realloc(buf->data, new_capa);
  if(new_data == NULL) return false;
  buf->data = new_data;
  buf->capa = new_capa;

  return true;
}

bool
mndb_str_buf_append(mndb_str_buf_t *buf, const char *data, ssize_t len_)
{
  size_t len;

  if(len_ < 0)
  {
    len = strlen(data);
  }
  else
  {
    len = (size_t) len_;
  }

  if(buf->capa < buf->len + len)
  {
    if(!mndb_str_buf_resize(buf, MAX(buf->capa * 2,
                                     buf->capa + buf->len + len)))
    {
      return false;
    }
  }

  memcpy(buf->data + buf->len, data, len);
  buf->len += len;

  return buf;
}

bool
mndb_str_buf_appendf(mndb_str_buf_t *buf, const char *format, ...)
{
  va_list vl;
  va_start(vl, format);

  size_t s;
  int n;
  int n_retries = 0;

retry:
  if(n_retries > 3) goto error;

  s = buf->capa - buf->len;
  n = vsnprintf(buf->data + buf->len, s, format, vl);

  if(n < 0) goto error;

  if((size_t)n > s)
  {
    if(!mndb_str_buf_resize(buf, MAX(buf->capa * 2,
                                     buf->capa + (size_t)n)))
    {
      goto error;
    }
    else
    {
      n_retries++;
      goto retry;
    }
  }
  else
  {
    // we don't want the nul byte
    buf->len += (size_t) n - 1;
  }

  va_end(vl);
  return true;

error:
  va_end(vl);
  return false;
}

void
mndb_str_buf_destroy(mndb_str_buf_t *buf)
{
  free(buf->data);
}

const char *
mndb_str_buf_data(mndb_str_buf_t *buf)
{
  return buf->data;
}

char *
mndb_str_buf_str(mndb_str_buf_t *buf)
{
  char *b = malloc(sizeof(char) * (buf->len + 1));
  memcpy(b, buf->data, buf->len);
  b[buf->len] = '\0';

  return b;
}
