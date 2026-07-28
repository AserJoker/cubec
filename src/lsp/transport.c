#include "lsp/transport.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

lsp_message_t *lsp_transport_read(void) {
  /* Read Content-Length header */
  size_t content_length = 0;
  char header_buf[256];

  for (;;) {
    if (!fgets(header_buf, sizeof(header_buf), stdin)) {
      return NULL; /* EOF */
    }

    /* Strip trailing \r\n */
    size_t len = strlen(header_buf);
    while (len > 0 && (header_buf[len - 1] == '\r' || header_buf[len - 1] == '\n')) {
      header_buf[--len] = '\0';
    }

    /* Empty line signals end of headers */
    if (len == 0) break;

    /* Parse Content-Length */
    if (strncmp(header_buf, "Content-Length:", 15) == 0) {
      content_length = (size_t)atol(header_buf + 15);
    }
  }

  if (content_length == 0) return NULL;

  /* Read body */
  char *body = (char *)malloc(content_length + 1);
  if (!body) return NULL;

  size_t read_total = 0;
  while (read_total < content_length) {
    size_t n = fread(body + read_total, 1, content_length - read_total, stdin);
    if (n == 0) {
      free(body);
      return NULL;
    }
    read_total += n;
  }
  body[content_length] = '\0';

  lsp_message_t *msg = (lsp_message_t *)malloc(sizeof(lsp_message_t));
  if (!msg) { free(body); return NULL; }
  msg->body = body;
  msg->body_len = content_length;
  return msg;
}

void lsp_transport_write(const char *json) {
  size_t len = strlen(json);
  fprintf(stdout, "Content-Length: %zu\r\n\r\n%s", len, json);
  fflush(stdout);
}

void lsp_message_free(lsp_message_t *msg) {
  if (!msg) return;
  free(msg->body);
  free(msg);
}
