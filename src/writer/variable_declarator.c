#include "writer/variable_declarator.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "core/location.h"
#include "writer/expression.h"

void cubec_write_variable_declarator(FILE *fp, cubec_ast_node_t node,
                                     cubec_write_context *ctx) {
  cubec_ast_node_t identifier = cubec_ast_get_child(node, "identifier");
  cubec_ast_node_t type = cubec_ast_get_child(node, "type");
  cubec_ast_node_t initialize = cubec_ast_get_child(node, "initialize");
  char *c_identifier = cubec_location_get(identifier->loc, ctx->allocator);
  fprintf(fp, "%s", c_identifier);
  if (type) {
    fprintf(fp, ": ");
    cubec_write_expression(fp, type, ctx);
  }
  if (initialize) {
    fprintf(fp, " = ");
    cubec_write_expression(fp, initialize, ctx);
  }
  cubec_allocator_free(ctx->allocator, c_identifier);
}