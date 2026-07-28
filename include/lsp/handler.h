#ifndef _H_CUBEC_LSP_HANDLER_
#define _H_CUBEC_LSP_HANDLER_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Handle an incoming LSP request/notification.
 * Returns a malloc'd JSON response string, or NULL if no response needed.
 * Caller must free the returned string.
 */
char *lsp_handle_request(const char *json_body);

/**
 * Initialize the handler (set up document store, etc.).
 */
void lsp_handler_init(void);

/**
 * Clean up handler resources.
 */
void lsp_handler_dispose(void);

#ifdef __cplusplus
}
#endif

#endif /* _H_CUBEC_LSP_HANDLER_ */
