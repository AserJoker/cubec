#include "resolve/statement_return.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/function.h"
#include "engine/interrupt.h"
#include "engine/value.h"
#include "resolve/expression.h"
value_t resolve_statement_return(context_t ctx, ast_node_t node) {
  ast_node_t value_node = ast_get_child(node, "value");
  value_t value = context_get_undefined(ctx);
  if (value_node) {
    value = resolve_expression(ctx, value_node);
    if (value_is_error(value)) {
      if (context_is_comptime(ctx)) {
        return value;
      } else {
        context_push_error(ctx, value);
        return context_get_undefined(ctx);
      }
    }
    if (value_is_interrupt(value)) {
      return value;
    }
  }
  allocator_t allocator = context_get_allocator(ctx);
  if (context_is_comptime(ctx)) {
    if (!value_is_comptime(value)) {
      return create_comptime_error(ctx, value_node, "value is not comptime");
    } else {
      return create_interrupt(ctx, value);
    }
  } else {
    value_t current_function = context_get_function(ctx);
    type_t type = value_get_type(current_function);
    type_t res_type = function_type_get_type(type);
    value = value_safe_convert(value, ctx, res_type);
    if (value_is_error(value)) {
      value = convert_comptime_error(ctx, value_node, value);
      context_push_error(ctx, value);
    }
    return context_get_undefined(ctx);
  }
}