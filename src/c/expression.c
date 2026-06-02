#include "c/expression.h"
#include "ast/node_type.h"
#include "c/value.h"

void c_expression(c_writer_t writer, ast_node_t node) {
  if (node->type == NODE_TYPE_VALUE) {
    c_value(writer, node->value);
  }
}