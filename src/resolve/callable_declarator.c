#include "resolve/callable_declarator.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/compare.h"
#include "core/hash.h"
#include "core/hash_map.h"
#include "core/location.h"
#include "engine/error.h"
#include "engine/function.h"
#include "engine/type.h"
#include "resolve/type.h"
#include <stdbool.h>
#include <string.h>

value_t resolve_callable_declarator(context_t ctx, ast_node_t node) {
  ast_node_t type = ast_get_child(node, "type");
  ast_node_t mut = ast_get_child(node, "mut");
  ast_node_t arguments = ast_get_child(node, "arguments");
  ast_node_t closure_node = ast_get_child(node, "closure");
  array_t args =
      create_array(ctx->allocator, &(array_initialize_t){.autofree = true});
  ctype_t ctype = NULL;
  bool variadic = false;
  for (size_t idx = 0; idx < ast_get_length(arguments); idx++) {
    ast_node_t arg = ast_get_item(arguments, idx);
    ast_node_t type = ast_get_child(arg, "type");
    ast_node_t mut = ast_get_child(arg, "mut");
    value_t value = resolve_type(ctx, type);
    if (value->type->kind == TYPE_KIND_ERROR) {
      allocator_free(ctx->allocator, args);
      return value;
    }
    type_t t = *(type_t *)value->data;
    ctype_t ctype = create_ctype(ctx->allocator, t, mut == NULL);
    array_push(args, ctype);
    if (arg->type == NODE_TYPE_CALLABLE_ARGUMENT_REST) {
      variadic = true;
    }
  }
  value_t vt = resolve_type(ctx, type);
  if (vt->type->kind == TYPE_KIND_ERROR) {
    allocator_free(ctx->allocator, args);
    return vt;
  }
  type_t t = *(type_t *)vt->data;
  ctype = create_ctype(ctx->allocator, t, mut == NULL);
  hash_map_t closure =
      create_hash_map(ctx->allocator, &(hash_map_initialize_t){
                                          .autofree_key = true,
                                          .autofree_value = false,
                                          .compare = (compare_fn_t)strcmp,
                                          .hash = (hash_fn_t)cstring_sdb,
                                      });
  for (size_t idx = 0; idx < ast_get_length(closure_node); idx++) {
    ast_node_t item = ast_get_item(closure_node, idx);
    ast_node_t identifier = ast_get_child(item, "identifier");
    ast_node_t type_node = ast_get_child(item, "type");
    value_t vtype = resolve_type(ctx, type_node);
    if (vtype->type->kind == TYPE_KIND_ERROR) {
      allocator_free(ctx->allocator, closure);
      allocator_free(ctx->allocator, args);
      allocator_free(ctx->allocator, ctype);
      return vtype;
    }
    type_t type = *(type_t *)vtype->data;
    char *name = location_get(node_get_location(identifier), ctx->allocator);
    if (hash_map_has(closure, name, NULL, NULL)) {
      allocator_free(ctx->allocator, closure);
      allocator_free(ctx->allocator, args);
      allocator_free(ctx->allocator, ctype);
      allocator_free(ctx->allocator, name);
      return create_comptime_error(ctx, node_get_location(item),
                                   "duplicate closure item");
    }
    hash_map_set(closure, name, type, NULL, NULL);
  }
  type_t function_type =
      create_function_type(ctx, ctype, args, variadic, closure);
  allocator_free(ctx->allocator, closure);
  allocator_free(ctx->allocator, args);
  allocator_free(ctx->allocator, ctype);
  return create_type_value(ctx, function_type, false, NULL);
}