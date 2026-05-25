#ifndef _H_ENGINE_FUNCTION_
#define _H_ENGINE_FUNCTION_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/hash_map.h"
#include "engine/context.h"
#include "engine/type.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef value_t (*handle_t)(context_t ctx, size_t argc, value_t *argv);
typedef struct _function_declar_t *function_declar_t;
typedef enum _function_kind_t {
  FUNCTION_KIND_NORMAL,
  FUNCTION_KIND_NATIVE,
  FUNCTION_KIND_EXTERN,
  FUNCTION_KIND_COMPTIME,
  FUNCTION_KIND_TEMPLATE,
} function_kind_t;
struct _function_declar_t {
  char *id;
  function_kind_t kind;
  type_t self;
  module_t mod;
  hash_map_t closure;
  union {
    ast_node_t node;
    handle_t handle;
    void *data;
  };
};
typedef struct _function_meta_t *function_meta_t;
struct _function_meta_t {
  ctype_t type;
  array_t args;
  bool variadic;
};
void init_template_type(context_t ctx);
function_declar_t create_function_declar(allocator_t allocator,
                                         function_kind_t kind, const char *id,
                                         type_t self, module_t mod);
type_t create_function_type(context_t ctx, ctype_t type, array_t argv,
                            bool variadic);
value_t create_function(context_t ctx, type_t type, ast_node_t node,
                        const char *id);
value_t function_add_closure(value_t self, context_t ctx, const char *name,
                             value_t value);
value_t create_template(context_t ctx, ast_node_t node);
value_t template_create_instance(value_t self, context_t ctx, size_t argc,
                                 value_t *argv);
value_t template_create_default_instance(value_t self, context_t ctx);
#ifdef __cplusplus
}
#endif
#endif