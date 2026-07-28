#ifndef _H_CUBEC_LSP_SERVER_
#define _H_CUBEC_LSP_SERVER_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Run the LSP server main loop. Blocks until exit notification received.
 * Returns exit code (0 for clean shutdown).
 */
int lsp_server_run(void);

#ifdef __cplusplus
}
#endif

#endif /* _H_CUBEC_LSP_SERVER_ */
