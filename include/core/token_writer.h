#ifndef _H_CUBEC_CORE_TOKEN_WRITER_
#define _H_CUBEC_CORE_TOKEN_WRITER_
#include "core/string.h"
#include "core/token.h"
#include "core/vec.h"
#ifdef __cplusplus
extern "C" {
#endif
#include <stddef.h>

/**
 * @brief Render an output token list to a string.
 *
 * Stateless: iterates the token list, concatenating token_get_string()
 * for each token. No indent computation — all formatting is baked
 * into the token list by the emit_* functions.
 */
string_t token_writer_render(allocator_t allocator, vec_t tokens);

#ifdef __cplusplus
}
#endif
#endif
