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
  bool comptime = location_is(kind->loc, "comptime");
  bool mutable = !location_is(kind->loc, "const");
  bool is_current_comptime = context_is_comptime(ctx);
  if (comptime) {
    context_set_comptime(ctx, true);
  }
  ast_node_t declarations = ast_get_child(node, "declarations");
  for (size_t idx = 0; idx < ast_get_length(declarations); idx++) {
    ast_node_t declar = ast_get_item(declarations, idx);
    value_t value = resolve_variable_declarator(ctx, declar);
    if (value_is_error(value)) {
      fprintf(stderr, "%s\n", error_get_message(value));
    }
    if (!mutable) {
      value_set_mutable(value, mutable);
    }
  }
  context_set_comptime(ctx, is_current_comptime);
  return context_get_undefined(ctx);
}