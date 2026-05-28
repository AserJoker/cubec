#include "resolve/callable_declarator.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/array.h"
#include "engine/function.h"
#include "engine/type.h"
#include "resolve/type.h"
#include <stdbool.h>

value_t resolve_callable_declarator(context_t ctx, ast_node_t node) {
  ast_node_t type = ast_get_child(node, "type");
  ast_node_t mut = ast_get_child(node, "mut");
  ast_node_t arguments = ast_get_child(node, "arguments");
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
  type_t function_type = create_function_type(ctx, ctype, args, variadic, NULL);
  allocator_free(ctx->allocator, args);
  allocator_free(ctx->allocator, ctype);
  return create_type_value(ctx, function_type, false, NULL);
}