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
void init_error_type(context_t ctx);
value_t create_error(context_t ctx, const char *fmt, ...);

value_t create_compile_error(context_t ctx, ast_node_t node, const char *fmt,
                             ...);
value_t convert_compile_error(context_t ctx, ast_node_t node, value_t err);
const char *error_get_message(value_t value);

#ifdef __cplusplus
}
#endif
#endif