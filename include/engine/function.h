#ifndef _H_ENGINE_FUNCTION_
#define _H_ENGINE_FUNCTION_
#include "ast/node.h"
#include "engine/context.h"
#include "engine/type.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _argument_t argument_t;
struct _argument_t {
  bool mut;
  type_t type;
};
void function_init(context_t ctx);
type_t create_function_type(context_t ctx, type_t type, size_t argc,
                            argument_t argv[], bool variadic);
value_t create_function(context_t ctx, type_t function_type, ast_node_t node);
value_t create_comptime_function(context_t ctx, ast_node_t node);
value_t create_template_function(context_t ctx, ast_node_t node);
type_t function_type_get_type(type_t self);
bool function_type_is_variadic(type_t self);
array_t function_type_get_arguments(type_t self);
value_t function_get_id(context_t ctx, value_t self);
value_t function_add_closure(context_t ctx, value_t self, const char *name,
                             value_t value);
#ifdef __cplusplus
}
#endif
#endif