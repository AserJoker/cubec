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

cubec_value_t cubec_create_function_type(cubec_context_t ctx, cubec_type_t type,
                                         size_t num_args, cubec_type_t args[],
                                         bool variadic);
cubec_array_t cubec_function_type_get_arguments(cubec_type_t self);
cubec_type_t cubec_function_type_get_type(cubec_type_t self);

bool cubec_function_type_is_variadic(cubec_type_t self);

cubec_value_t cubec_create_function(cubec_context_t ctx, cubec_type_t func_type,
                                    cubec_ast_node_t func, bool mutable,
                                    const char *name);

#ifdef __cplusplus
}
#endif
#endif