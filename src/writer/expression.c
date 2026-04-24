#include "writer/expression.h"
#include "ast/node_type.h"
#include "writer/expression_binary.h"
#include "writer/expression_call.h"
#include "writer/literal_identifier.h"
#include "writer/value.h"
void write_expression(allocator_t allocator, ast_node_t node, stream_t stream) {
  if (node->type == NODE_TYPE_VALUE) {
    return write_value(allocator, node, stream);
  } else if (node->type == NODE_TYPE_EXPRESSION_BINARY) {
    return write_expression_binary(allocator, node, stream);
  } else if (node->type == NODE_TYPE_LITERAL_IDENTIFIER) {
    return write_literal_identifier(allocator, node, stream);
  } else if (node->type == NODE_TYPE_EXPRESSION_CALL) {
    return write_expression_call(allocator, node, stream);
  }
}