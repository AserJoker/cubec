#ifndef _H_CUBEC_LSP_DIAGNOSTICS_
#define _H_CUBEC_LSP_DIAGNOSTICS_

#include <cJSON.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Run lex+parse+check on the given source and return LSP Diagnostic[] JSON array.
 * uri is used for the diagnostic's range file reference.
 * source is the file content.
 * Returns a cJSON array that the caller owns (must cJSON_Delete).
 */
cJSON *lsp_diagnostics_for_file(const char *uri, const char *source);

#ifdef __cplusplus
}
#endif

#endif /* _H_CUBEC_LSP_DIAGNOSTICS_ */
