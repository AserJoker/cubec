#ifndef _H_CUBEC_ENGINE_ERROR_
#define _H_CUBEC_ENGINE_ERROR_
#include "ast/node.h"
#include "engine/context.h"
#include "engine/value.h"
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void cubec_init_error_type(cubec_context_t ctx);
cubec_value_t cubec_create_error(cubec_context_t ctx, const char *fmt, ...);

cubec_value_t cubec_create_compile_error(cubec_context_t ctx,
                                         cubec_ast_node_t node, const char *fmt,
                                         ...);
cubec_value_t cubec_convert_compile_error(cubec_context_t ctx,
                                          cubec_ast_node_t node,
                                          cubec_value_t err);

#ifdef __cplusplus
}
#endif
#endif