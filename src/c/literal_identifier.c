#include "c/literal_identifier.h"
#include "ast/node.h"
#include "core/stream.h"
#include "engine/type.h"
#include "engine/value.h"
void write_c_literal_identifier(context_t ctx, ast_node_t node,
                                stream_t stream) {
  ast_node_t _value = ast_get_child(node, "_value");
  if (_value) {
    value_t value = _value->value;
    type_t self = value_get_self(value);
    if (self) {
      stream_write(stream, type_get_id(self));
      stream_write(stream, "V");
    }
  }
  stream_write_location(stream, node->loc);
}