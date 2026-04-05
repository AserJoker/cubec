#include "writer/statement_import.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "core/location.h"
#include <stdio.h>
void cubec_write_statement_import(FILE *fp, cubec_ast_node_t node,
                                  cubec_write_context *ctx) {
  cubec_ast_node_t identifier = cubec_ast_get_child(node, "identifier");
  cubec_ast_node_t source = cubec_ast_get_child(node, "source");
  char *c_identifier = cubec_location_get(identifier->loc, ctx->allocator);
  char *c_source = cubec_location_get(source->loc, ctx->allocator);
  fprintf(fp, "import %s from %s;\n", c_identifier, c_source);
  cubec_allocator_free(ctx->allocator, c_source);
  cubec_allocator_free(ctx->allocator, c_identifier);
}