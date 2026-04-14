#include "resolve/program.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/struct.h"
#include "engine/value.h"
#include "resolve/statement_declaration.h"
#include <stdalign.h>
#include <stdio.h>

cubec_value_t cubec_resolve_program(cubec_context_t ctx,
                                    cubec_ast_node_t node) {
  cubec_ast_node_t statements = cubec_ast_get_child(node, "statements");
  cubec_value_t program =
      cubec_create_struct_type(ctx, alignof(struct {}), NULL);
  cubec_context_push_static_scope(ctx, program);
  for (size_t idx = 0; idx < cubec_ast_get_length(statements); idx++) {
    cubec_ast_node_t sts = cubec_ast_get_item(statements, idx);
    if (sts->type == CUBEC_NODE_TYPE_STATEMENT_DECLARATION) {
      cubec_resolve_statement_declaration(ctx, sts);
    } else {
      cubec_value_t err =
          cubec_create_compile_error(ctx, sts, "unsupport top statement");
      fprintf(stderr, "%s\n", cubec_error_get_message(err));
    }
  }
  cubec_context_pop_static_scope(ctx);
  return program;
}