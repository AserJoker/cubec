#ifndef _H_ENGINE_FUNCTION_
#define _H_ENGINE_FUNCTION_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/array.h"
#include "engine/context.h"
#include "engine/type.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _argument_t *argument_t;
struct _argument_t {
  bool mut;
  type_t type;
};

typedef struct _closure_item_t {
  char *name;
  value_t value;
} *closure_item_t;

typedef value_t (*native_handle_t)(context_t ctx, size_t argc, value_t *argv);

typedef enum _func_type_t {
  FUNC_TYPE_AST,
  FUNC_TYPE_NATIVE,
} func_type_t;

typedef struct _function_declar_t {
  union {
    ast_node_t node;
    native_handle_t handle;
    void *data;
  };
  func_type_t type;
  type_t global;
  type_t bind;
  char *id;
  array_t closure;
} *function_declar_t;

void function_init(context_t ctx);
type_t create_function_type(context_t ctx, type_t type, array_t arguments,
                            bool variadic);
value_t create_function(context_t ctx, type_t function_type, ast_node_t node,
                        array_t closure);
value_t create_comptime_function(context_t ctx, ast_node_t node,
                                 array_t closure);
value_t create_template_function(context_t ctx, ast_node_t node,
                                 array_t closure);
type_t function_type_get_type(type_t self);
bool function_type_is_variadic(type_t self);
array_t function_type_get_arguments(type_t self);
value_t function_get_id(context_t ctx, value_t self);
closure_item_t create_closure_item(allocator_t allocaotr, const char *name,
                                   value_t value);
function_declar_t create_function_declar(allocator_t allocator,
                                         func_type_t type, type_t global,
                                         type_t self, const char *id,
                                         void *node, array_t closure);
#ifdef __cplusplus
}
#endif
#endif