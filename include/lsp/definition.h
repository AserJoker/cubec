#ifndef _H_CUBEC_LSP_DEFINITION_
#define _H_CUBEC_LSP_DEFINITION_

#include <cJSON.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Find the definition location for a symbol at the given position.
 * uri: document URI (file:// scheme)
 * line: 0-based line number
 * character: 0-based column number
 * Returns a cJSON Location object (or null), caller must cJSON_Delete.
 */
cJSON *lsp_definition_for_position(const char *uri, int line, int character);

#ifdef __cplusplus
}
#endif

#endif /* _H_CUBEC_LSP_DEFINITION_ */
