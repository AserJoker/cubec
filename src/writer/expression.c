#include "writer/expression.h"
#include "ast/node_type.h"
#include "writer/literal_char.h"
#include "writer/literal_identifier.h"
#include "writer/literal_number.h"
#include "writer/literal_string.h"
void cubec_write_expression(FILE *fp, cubec_ast_node_t node,
                            cubec_write_context *ctx) {
  if (node->type == CUBEC_NODE_TYPE_LITERAL_CHAR) {
    cubec_write_literal_char(fp, node, ctx);
  } else if (node->type == CUBEC_NODE_TYPE_LITERAL_IDENTIFIER) {
    cubec_write_literal_identifier(fp, node, ctx);
  } else if (node->type == CUBEC_NODE_TYPE_LITERAL_NUMERIC) {
    cubec_write_literal_number(fp, node, ctx);
  } else if (node->type == CUBEC_NODE_TYPE_LITERAL_STRING) {
    cubec_write_literal_string(fp, node, ctx);
  }
  // TODO:
}