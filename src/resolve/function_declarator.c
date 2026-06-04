#include "resolve/function_declarator.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/function.h"
#include "engine/type.h"
#include <stdbool.h>

value_t resolve_function_declarator(context_t ctx, ast_node_t node) {
  ast_node_t closure = ast_get_child(node, "closure");
  value_t func = create_template(ctx, node);
  for (size_t idx = 0; idx < ast_get_length(closure); idx++) {
    ast_node_t item = ast_get_item(closure, idx);
    if (item->type == NODE_TYPE_LITERAL_IDENTIFIER) {
      char *name = location_get(node_get_location(item), ctx->allocator);
      value_t value = context_load(ctx, name);
      if (value->type->kind == TYPE_KIND_ERROR) {
        allocator_free(ctx->allocator, name);
        return value;
      }
      value_t err = function_add_closure(func, ctx, name, value);
      allocator_free(ctx->allocator, name);
      if (err->type->kind == TYPE_KIND_ERROR) {
        return err;
      }
    }
  }
  value_t ins = template_create_default_instance(func, ctx);
  if (ins) {
    func = ins;
  }
  if (ast_get_length(closure) && func->type->kind == TYPE_KIND_FUNCTION) {
    ast_node_t bind = create_ast_value(ctx->allocator, func);
    ast_add_child(ctx->allocator, node, "bind", bind);
    return context_create_value(ctx, func->type, false, NULL);
  }
  return func;
}