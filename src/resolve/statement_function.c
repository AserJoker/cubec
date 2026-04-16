#include "resolve/statement_function.h"
#include "ast/node.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/value.h"
#include "resolve/function_declarator.h"
#include <stdbool.h>
#include <stdio.h>

value_t resolve_statement_function(context_t ctx, ast_node_t node) {
  ast_node_t function = ast_get_child(node, "function");
  value_t func = resolve_function_declarator(ctx, function);
  if (value_is_error(func)) {
    fprintf(stderr, "%s\n", error_get_message(func));
  }
  return context_get_undefined(ctx);
}