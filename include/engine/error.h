#ifndef _H_ENGINE_ERROR_
#define _H_ENGINE_ERROR_
#include "ast/node.h"
#include "engine/context.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
void error_init(context_t ctx);
value_t create_error(context_t ctx, const char *fmt, ...);
value_t create_compile_error(context_t ctx, ast_node_t node, const char *fmt,
                             ...);
value_t convert_compile_error(context_t ctx, ast_node_t node, value_t err);
const char *error_get_message(value_t self);
#ifdef __cplusplus
}
#endif
#endif