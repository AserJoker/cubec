#include "c/expression.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "c/expression_member.h"
#include "c/literal_identifier.h"
#include "c/value.h"
#include "engine/value.h"
void write_c_expression(context_t ctx, ast_node_t node, stream_t stream) {
  ast_node_t _value = NULL;
  if (node->type > NODE_TYPE_LIST) {
    _value = ast_get_child(node, "_value");
  }
  if (_value && value_is_comptime(_value->value)) {
    write_c_value(ctx, _value->value, stream);
  } else if (node->type == NODE_TYPE_VALUE) {
    write_c_value(ctx, node->value, stream);
  } else if (node->type == NODE_TYPE_EXPRESSION_MEMBER) {
    write_c_expression_member(ctx, node, stream);
  } else if (node->type == NODE_TYPE_LITERAL_IDENTIFIER) {
    write_c_literal_identifier(ctx, node, stream);
  }
}