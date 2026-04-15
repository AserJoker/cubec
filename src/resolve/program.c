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

value_t resolve_program(context_t ctx, ast_node_t node) {
  ast_node_t statements = ast_get_child(node, "statements");
  value_t program = create_struct_type(ctx, alignof(struct {}), NULL);
  context_push_static_scope(ctx, program);
  for (size_t idx = 0; idx < ast_get_length(statements); idx++) {
    ast_node_t sts = ast_get_item(statements, idx);
    if (sts->type == NODE_TYPE_STATEMENT_DECLARATION) {
      resolve_statement_declaration(ctx, sts);
    } else {
      value_t err = create_compile_error(ctx, sts, "unsupport top statement");
      fprintf(stderr, "%s\n", error_get_message(err));
    }
  }
  context_pop_static_scope(ctx);
  return program;
}