#include "resolve/statement_declaration.h"
#include "ast/node.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/value.h"
#include "resolve/variable_declarator.h"
#include <stdio.h>
cubec_value_t cubec_resolve_statement_declaration(cubec_context_t ctx,
                                                  cubec_ast_node_t node) {
  cubec_ast_node_t kind = cubec_ast_get_child(node, "kind");
  bool comptime = cubec_location_is(kind->loc, "comptime");
  bool mutable = !cubec_location_is(kind->loc, "const");
  bool is_current_comptime = cubec_context_is_comptime(ctx);
  if (comptime) {
    cubec_context_set_comptime(ctx, true);
  }
  cubec_ast_node_t declarations = cubec_ast_get_child(node, "declarations");
  for (size_t idx = 0; idx < cubec_ast_get_length(declarations); idx++) {
    cubec_ast_node_t declar = cubec_ast_get_item(declarations, idx);
    cubec_value_t value = cubec_resolve_variable_declarator(ctx, declar);
    if (cubec_value_is_error(value)) {
      fprintf(stderr, "%s\n", cubec_error_get_message(value));
    }
    if (!mutable) {
      cubec_value_set_mutable(value, mutable);
    }
  }
  cubec_context_set_comptime(ctx, is_current_comptime);
  return cubec_context_get_undefined(ctx);
}