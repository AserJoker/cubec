#include "resolve/statement_declaration.h"
#include "ast/node.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/value.h"
#include "resolve/variable_declarator.h"
#include <stdio.h>
value_t resolve_statement_declaration(context_t ctx, ast_node_t node) {
  ast_node_t kind = ast_get_child(node, "kind");
  ast_node_t type = ast_get_child(node, "type");
  bool comptime = kind && location_is(kind->loc, "comptime");
  bool current = context_set_comptime(ctx, comptime);
  ast_node_t declarations = ast_get_child(node, "declarations");
  for (size_t idx = 0; idx < ast_get_length(declarations); idx++) {
    ast_node_t declar = ast_get_item(declarations, idx);
    value_t value = resolve_variable_declarator(ctx, declar);
    if (value_is_error(value)) {
      fprintf(stderr, "%s\n", error_get_message(value));
    }
    if (location_is(type->loc, "let")) {
      value_set_mutable(value, true);
    }
  }
  context_set_comptime(ctx, current);
  return context_get_undefined(ctx);
}