#include "fmt/expression.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "fmt/callable_declarator.h"
#include "fmt/expression_assigment.h"
#include "fmt/expression_binary.h"
#include "fmt/expression_call.h"
#include "fmt/expression_compute_member.h"
#include "fmt/expression_member.h"
#include "fmt/function_declarator.h"
#include "fmt/initialize_list.h"
#include "fmt/literal_identifier.h"
#include "fmt/literal_numeric.h"
#include "fmt/literal_string.h"
#include "fmt/ptr_declarator.h"
#include "fmt/struct_declarator.h"
void fmt_expression(allocator_t allocator, ast_node_t node, stream_t stream) {
  if (node->type == NODE_TYPE_EXPRESSION_BINARY) {
    fmt_expression_binary(allocator, node, stream);
  } else if (node->type == NODE_TYPE_LITERAL_IDENTIFIER) {
    fmt_literal_identifier(allocator, node, stream);
  } else if (node->type == NODE_TYPE_EXPRESSION_CALL) {
    fmt_expression_call(allocator, node, stream);
  } else if (node->type == NODE_TYPE_EXPRESSION_MEMBER) {
    fmt_expression_member(allocator, node, stream);
  } else if (node->type == NODE_TYPE_INITIALIZE_LIST) {
    fmt_initialize_list(allocator, node, stream);
  } else if (node->type == NODE_TYPE_FUNCTION_DECLARATOR) {
    fmt_function_delcarator(allocator, node, stream);
  } else if (node->type == NODE_TYPE_STRUCT_DECLARATOR) {
    fmt_struct_declarator(allocator, node, stream);
  } else if (node->type == NODE_TYPE_PTR_DECLARATOR) {
    fmt_ptr_declarator(allocator, node, stream);
  } else if (node->type == NODE_TYPE_LITERAL_STRING) {
    fmt_literal_string(allocator, node, stream);
  } else if (node->type == NODE_TYPE_LITERAL_NUMERIC) {
    fmt_literal_numeric(allocator, node, stream);
  } else if (node->type == NODE_TYPE_CALLABLE_DECLARATOR) {
    fmt_callable_declarator(allocator, node, stream);
  } else if (node->type == NODE_TYPE_EXPRESSION_COMPUTE_MEMBER) {
    fmt_expression_compute_member(allocator, node, stream);
  } else if (node->type == NODE_TYPE_EXPRESSION_ASSIGMENT) {
    fmt_expression_assigment(allocator, node, stream);
  }
}