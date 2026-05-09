#include "c/expression.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "c/value.h"
void write_c_expression(context_t ctx, ast_node_t node, stream_t stream) {
  if (node->type > NODE_TYPE_LIST) {
    ast_node_t _value = ast_get_child(node, "_value");
    if (_value) {
      write_c_value(ctx, _value->value, stream);
    }
  } else if (node->type == NODE_TYPE_VALUE) {
    write_c_value(ctx, node->value, stream);
  }
}