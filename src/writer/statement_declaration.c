#include "writer/statement_declaration.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "core/location.h"
#include "writer/variable_declarator.h"
#include <stdio.h>
void cubec_write_statement_declaration(FILE *fp, cubec_ast_node_t node,
                                       cubec_write_context *ctx) {
  if (ctx->indent) {
    fprintf(fp, "%*s", (int)ctx->indent * 4, " ");
  }
  cubec_ast_node_t kind = cubec_ast_get_child(node, "kind");
  char *c_kind = cubec_location_get(kind->loc, ctx->allocator);
  fprintf(fp, "%s ", c_kind);
  cubec_ast_node_t declarations = cubec_ast_get_child(node, "declarations");
  for (size_t idx = 0; idx < cubec_ast_get_length(declarations); idx++) {
    if (idx != 0) {
      fprintf(fp, ", ");
    }
    cubec_ast_node_t declarator = cubec_ast_get_item(declarations, idx);
    cubec_write_variable_declarator(fp, declarator, ctx);
  }
  fprintf(fp, ";");
  cubec_allocator_free(ctx->allocator, c_kind);
}