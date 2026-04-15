#ifndef _H_CUBEC_ENGINE_FUNCTION_
#define _H_CUBEC_ENGINE_FUNCTION_
#include "ast/node.h"
#include "core/array.h"
#include "engine/context.h"
#include "engine/type.h"
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

value_t create_function_type(context_t ctx, type_t type, size_t num_args,
                             type_t args[], bool variadic);
array_t function_type_get_arguments(type_t self);
type_t function_type_get_type(type_t self);

bool function_type_is_variadic(type_t self);

value_t create_function(context_t ctx, type_t func_type, ast_node_t func,
                        bool mutable, const char *name);

#ifdef __cplusplus
}
#endif
#endif