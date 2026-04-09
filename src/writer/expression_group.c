#include "writer/expression_group.h"
#include "ast/node.h"
#include "writer/expression.h"
void cubec_write_expression_group(FILE *fp, cubec_ast_node_t node,
                                  cubec_write_context *ctx) {
  cubec_ast_node_t body = cubec_ast_get_child(node, "expression");
  fprintf(fp, "(");
  cubec_write_expression(fp, body, ctx);
  fprintf(fp, ")");
}