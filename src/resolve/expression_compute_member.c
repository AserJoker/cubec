#include "resolve/expression_compute_member.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/function.h"
#include "engine/struct.h"
#include "engine/type.h"
#include "engine/value.h"
#include "resolve/expression.h"

value_t resolve_expression_compute_member(context_t ctx, ast_node_t node) {
  ast_node_t host = ast_get_child(node, "host");
  ast_node_t field = ast_get_child(node, "field");
  value_t obj = NULL;
  obj = resolve_expression(ctx, host);
  if (obj->type->kind == TYPE_KIND_ERROR) {
    return obj;
  }
  value_t f = resolve_expression(ctx, field);
  if (f->type->kind == TYPE_KIND_ERROR) {
    return f;
  }
  if (obj->type->kind == TYPE_KIND_STRUCT) {
    type_t stru = obj->type;
    struct_attribute_t attr = struct_type_get_method(stru, "__get__");
    if (!attr) {
      return create_comptime_error(ctx, node_get_location(node),
                                   "no member __get__ in '%s'", stru->name);
    }
    value_t func = NULL;
    if (attr->initialize->type == NODE_TYPE_VALUE) {
      func = attr->initialize->value;
    } else {
      ast_node_t bind = ast_get_child(attr->initialize, "bind");
      func = bind->value;
    }
    value_t self = value_addr(obj, ctx);
    value_t args[] = {self, f};
    if (func->type->kind == TYPE_KIND_TEMPLATE) {
      func = template_create_instance(func, ctx, 2, args);
      if (func->type->kind == TYPE_KIND_ERROR) {
        return convert_comptime_error(ctx, node_get_location(node), func);
      }
    }
    ast_node_t bind = create_ast_value(ctx->allocator, func);
    ast_add_child(ctx->allocator, node, "bind", bind);
    return value_call(func, ctx, 2, args);
  }
  value_t result = value_get(obj, ctx, f);
  if (result->type->kind == TYPE_KIND_ERROR) {
    return convert_comptime_error(ctx, node_get_location(node), result);
  }
  if (ctx->comptime && !result->comptime) {
    return create_comptime_error(ctx, node_get_location(node),
                                 "value is not comptime");
  }
  return result;
}