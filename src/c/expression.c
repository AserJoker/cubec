#include "c/expression.h"
#include "ast/node_type.h"
#include "c/initialize_list.h"
#include "c/value.h"

void c_expression(c_writer_t writer, ast_node_t node) {
  if (node->type == NODE_TYPE_VALUE) {
    c_value(writer, node->value);
  } else if (node->type == NODE_TYPE_INITIALIZE_LIST) {
    c_initialize_list(writer, node);
  }
}