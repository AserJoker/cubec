#ifndef _H_ENGINE_CONTEXT_
#define _H_ENGINE_CONTEXT_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/hash_map.h"
#include "core/string.h"
#include "engine/module.h"
#include "engine/scope.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum _context_type_t {
  CONTEXT_TYPE_FUNCTION,
  CONTEXT_TYPE_GENERATOR,
  CONTEXT_TYPE_ASYNC,
  CONTEXT_TYPE_STRUCT,
  CONTEXT_TYPE_UNION,
} context_type_t;

typedef struct _context_t *context_t;
typedef value_t (*function_handle_t)(context_t ctx, size_t argc, value_t *argv);
typedef struct _function_declar {
  ast_node_t node;
  function_handle_t native;
  type_t global;
  type_t bind;
  const char *id;
  hash_map_t closure;
} *function_declar;

typedef ast_node_t (*builtin_fn_t)(context_t ctx, size_t argc,
                                   ast_node_t *argv);
context_t create_context(allocator_t allocator);
bool context_is_comptime(context_t ctx);
bool context_set_comptime(context_t ctx, bool comptime);
type_t context_get_global(context_t ctx);
type_t context_set_global(context_t ctx, type_t global);
context_type_t context_get_type(context_t ctx);
context_type_t context_set_type(context_t ctx, context_type_t type);
void context_push_scope(context_t self);
void context_pop_scope(context_t self);
scope_t context_get_scope(context_t self);
void context_set_scope(context_t self, scope_t scope);
scope_t context_get_root_scope(context_t self);
void context_set_root_scope(context_t self, scope_t scope);
void context_set_builtin(context_t ctx, const char *name, builtin_fn_t fn);
ast_node_t context_eval_builtin(context_t ctx, const char *name, size_t argc,
                                ast_node_t *argv);
bool context_has_builtin(context_t ctx, const char *name);
const char *context_create_cstring(context_t self, const char *src);
allocator_t context_get_allocator(context_t self);
value_t context_load(context_t self, const char *name);
value_t context_declar(context_t self, const char *name, value_t value);
value_t context_get_undefined(context_t self);
value_t context_get_true(context_t self);
value_t context_get_false(context_t self);
value_t context_create_value(context_t self, type_t type, const void *data,
                             bool mut, bool comptime, const char *name);
value_t context_create_weak_value(context_t self, type_t type, void *data,
                                  bool mut, const char *name);
value_t context_load_module(context_t self, const char *filename);
void context_push_error(context_t self, value_t error);
void context_store_type(context_t self, type_t type);
type_t context_load_type(context_t self, const char *id);
value_t context_clone_value(context_t self, value_t value);
string_t context_fmt_module(context_t self, const char *module);
module_t context_get_module(context_t self);
type_t context_get_self(context_t self);
type_t context_set_self(context_t ctx, type_t self);
value_t context_set_function(context_t ctx, value_t function);
value_t context_get_function(context_t ctx);

function_declar context_load_function_declar(context_t self, const char *id);
function_declar context_store_function_declar(context_t self, ast_node_t node,
                                              const char *id);
function_declar context_store_native_declar(context_t self,
                                            function_handle_t native,
                                            const char *id);
#ifdef __cplusplus
}
#endif
#endif