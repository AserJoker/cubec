#include "eval/statement_declaration.h"
#include "ast/node.h"
#include "engine/context.h"
cubec_value_t cubec_eval_statement_declaration(cubec_context_t ctx,
                                               cubec_ast_node_t node) {
  cubec_ast_node_t kind = cubec_ast_get_child(node, "kind");
  cubec_ast_node_t declarators = cubec_ast_get_child(node, "declarators");
  // TODO:
  return cubec_context_get_undefined(ctx);
}