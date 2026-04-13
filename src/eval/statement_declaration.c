#include "eval/statement_declaration.h"
#include "ast/node.h"
#include "engine/context.h"
#include "engine/value.h"
#include "eval/variable_declaratior.h"

cubec_value_t cubec_eval_statement_declaration(cubec_context_t ctx,
                                               cubec_ast_node_t node) {
  cubec_ast_node_t kind = cubec_ast_get_child(node, "kind");
  cubec_ast_node_t declarations = cubec_ast_get_child(node, "declarations");
  for (size_t idx = 0; idx < cubec_ast_get_length(declarations); idx++) {
    cubec_ast_node_t declaration = cubec_ast_get_item(declarations, idx);
    cubec_value_t value =
        cubec_eval_variable_declaratior(ctx, declaration, kind);
    if (cubec_value_is_error(value)) {
      return value;
    }
  }
  return cubec_context_get_undefined(ctx);
}