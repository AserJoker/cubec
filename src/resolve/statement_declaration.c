#include "resolve/statement_declaration.h"
#include "ast/node.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/value.h"
#include "resolve/variable_declarator.h"
#include <stdio.h>
cubec_value_t cubec_resolve_statement_declaration(cubec_context_t ctx,
                                                  cubec_ast_node_t node) {
  cubec_ast_node_t kind = cubec_ast_get_child(node, "kind");
  cubec_ast_node_t declarations = cubec_ast_get_child(node, "declarations");
  for (size_t idx = 0; idx < cubec_ast_get_length(declarations); idx++) {
    cubec_ast_node_t declar = cubec_ast_get_item(declarations, idx);
    cubec_value_t value = cubec_resolve_variable_declarator(ctx, declar, kind);
    if (cubec_value_is_error(value)) {
      fprintf(stderr, "%s\n", cubec_error_get_message(value));
    }
  }
  return cubec_context_get_undefined(ctx);
}