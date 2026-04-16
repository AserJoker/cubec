#include "resolve/statement_return.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/function.h"
#include "engine/type.h"
#include "engine/value.h"
#include "resolve/expression.h"
#include <stdio.h>

value_t resolve_statement_return(context_t ctx, ast_node_t node) {
  ast_node_t value_node = ast_get_child(node, "value");
  value_t value = NULL;
  if (value_node) {
    value = resolve_expression(ctx, value_node);
  } else {
    value = context_get_undefined(ctx);
  }
  if (value_is_error(value)) {
    fprintf(stderr, "%s\n", error_get_message(value));
    return context_get_undefined(ctx);
  }
  static_scope_t scope = context_get_static_scope(ctx);
  if (!value_type_is(scope->binding, VALUE_TYPE_FUNCTION)) {
    value_t err = create_compile_error(
        ctx, node, "return statement only used in function");
    fprintf(stderr, "%s\n", error_get_message(err));
    return context_get_undefined(ctx);
  }
  type_t func_type = value_get_type(scope->binding);
  type_t return_type = function_type_get_type(func_type);
  type_t value_type = value_get_type(value);
  allocator_t allocator = context_get_allocator(ctx);
  if (!type_is_safe_convert(value_type, return_type)) {
    char *dst_type_name = type_to_string(return_type, allocator);
    char *src_type_name = type_to_string(value_type, allocator);
    value_t err =
        create_compile_error(ctx, value_node, "cannot convert '%s' to '%s' ",
                             src_type_name, dst_type_name);
    allocator_free(allocator, dst_type_name);
    allocator_free(allocator, src_type_name);
    return context_get_undefined(ctx);
  }
  return context_get_undefined(ctx);
}