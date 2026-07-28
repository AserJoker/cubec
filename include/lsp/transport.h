#ifndef _H_CUBEC_LSP_TRANSPORT_
#define _H_CUBEC_LSP_TRANSPORT_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  char *body;      /* JSON-RPC message body (malloc'd, null-terminated) */
  size_t body_len; /* length of body (excluding null terminator) */
} lsp_message_t;

/**
 * Read one LSP message from stdin (Content-Length header + body).
 * Returns NULL on EOF. Caller must free with lsp_message_free().
 */
lsp_message_t *lsp_transport_read(void);

/**
 * Write a JSON-RPC response to stdout with Content-Length header.
 */
void lsp_transport_write(const char *json);

/**
 * Free an lsp_message_t.
 */
void lsp_message_free(lsp_message_t *msg);

#ifdef __cplusplus
}
#endif

#endif /* _H_CUBEC_LSP_TRANSPORT_ */
