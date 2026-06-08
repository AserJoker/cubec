#include "c/expression.h"
#include "ast/node_type.h"
#include "c/expression_binary.h"
#include "c/expression_call.h"
#include "c/expression_member.h"
#include "c/function_declarator.h"
#include "c/initialize_list.h"
#include "c/literal_identifier.h"
#include "c/value.h"

void c_expression(c_writer_t writer, ast_node_t node) {
  if (node->type == NODE_TYPE_VALUE) {
    c_value(writer, node->value);
  } else if (node->type == NODE_TYPE_INITIALIZE_LIST) {
    c_initialize_list(writer, node);
  } else if (node->type == NODE_TYPE_LITERAL_IDENTIFIER) {
    c_literal_identifier(writer, node);
  } else if (node->type == NODE_TYPE_EXPRESSION_BINARY) {
    c_expression_binary(writer, node);
  } else if (node->type == NODE_TYPE_FUNCTION_DECLARATOR) {
    c_function_declarator(writer, node);
  } else if (node->type == NODE_TYPE_EXPRESSION_CALL) {
    c_expression_call(writer, node);
  } else if (node->type == NODE_TYPE_EXPRESSION_MEMBER) {
    c_expression_member(writer, node);
  }
}