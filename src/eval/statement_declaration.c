#include "eval/statement_declaration.h"
#include "ast/node.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/value.h"
#include "eval/variable_declaratior.h"

cubec_value_t cubec_eval_statement_declaration(cubec_context_t ctx,
                                               cubec_ast_node_t node) {
  cubec_ast_node_t kind = cubec_ast_get_child(node, "kind");
  cubec_ast_node_t declarations = cubec_ast_get_child(node, "declarations");
  for (size_t idx = 0; idx < cubec_ast_get_length(declarations); idx++) {
    cubec_ast_node_t declaration = cubec_ast_get_item(declarations, idx);
    cubec_value_t value = cubec_eval_variable_declaratior(ctx, declaration);
    if (cubec_value_is_error(value)) {
      return value;
    }
    if (cubec_location_is(kind->loc, "const")) {
      cubec_value_set_mutable(value, false);
    } else if (cubec_location_is(kind->loc, "comptime")) {
      if (!cubec_value_get_data(value)) {
        return cubec_create_compile_error(ctx, declaration,
                                          "value is not comptime");
      }
    }
  }
  return cubec_context_get_undefined(ctx);
}