#include "writer/expression.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "writer/expression_binary.h"
#include "writer/expression_call.h"
#include "writer/expression_member.h"
#include "writer/function_declarator.h"
#include "writer/initialize_list.h"
#include "writer/literal_identifier.h"
#include "writer/literal_numeric.h"
#include "writer/literal_string.h"
#include "writer/ptr_declarator.h"
#include "writer/struct_declarator.h"
void write_expression(allocator_t allocator, ast_node_t node, stream_t stream) {
  if (node->type == NODE_TYPE_EXPRESSION_BINARY) {
    write_expression_binary(allocator, node, stream);
  } else if (node->type == NODE_TYPE_LITERAL_IDENTIFIER) {
    write_literal_identifier(allocator, node, stream);
  } else if (node->type == NODE_TYPE_EXPRESSION_CALL) {
    write_expression_call(allocator, node, stream);
  } else if (node->type == NODE_TYPE_EXPRESSION_MEMBER) {
    write_expression_member(allocator, node, stream);
  } else if (node->type == NODE_TYPE_INITIALIZE_LIST) {
    write_initialize_list(allocator, node, stream);
  } else if (node->type == NODE_TYPE_FUNCTION_DECLARATOR) {
    write_function_delcarator(allocator, node, stream);
  } else if (node->type == NODE_TYPE_STRUCT_DECLARATOR) {
    write_struct_declarator(allocator, node, stream);
  } else if (node->type == NODE_TYPE_PTR_DECLARATOR) {
    write_ptr_declarator(allocator, node, stream);
  } else if (node->type == NODE_TYPE_LITERAL_STRING) {
    write_literal_string(allocator, node, stream);
  } else if (node->type == NODE_TYPE_LITERAL_NUMERIC) {
    write_literal_numeric(allocator, node, stream);
  }
}