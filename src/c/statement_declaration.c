#include "c/statement_declaration.h"
#include "ast/node.h"
#include "c/expression.h"
#include "c/value.h"
#include "c/variable_declarator.h"
#include "core/location.h"
#include "core/stream.h"
void write_c_statement_declaration(context_t ctx, ast_node_t node,
                                   stream_t stream) {
  ast_node_t kind = ast_get_child(node, "kind");
  ast_node_t declarations = ast_get_child(node, "declarations");
  ast_node_t declar_type = ast_get_child(node, "type");
  if (kind && location_is(kind->loc, "comptime")) {
    return;
  }
  for (size_t idx = 0; idx < ast_get_length(declarations); idx++) {
    if (idx != 0) {
      stream_write(stream, ", ");
    }
    if (location_is(declar_type->loc, "const")) {
      stream_write(stream, "const ");
    }
    ast_node_t declar = ast_get_item(declarations, idx);
    ast_node_t _type = ast_get_child(declar, "_type");
    ast_node_t initialize = ast_get_child(declar, "initialize");
    ast_node_t identifier = ast_get_child(declar, "identifier");
    value_t vtype = _type->value;
    type_t type = *(type_t *)value_get_data(vtype);
    write_c_declar(ctx, type, identifier, stream);
    stream_write(stream, " = ");
    if (initialize->type == NODE_TYPE_VALUE) {
      value_t value = initialize->value;
      value = context_create_value(ctx, type, value_get_data(value),
                                   !location_is(declar_type->loc, "const"),
                                   true, NULL);
      write_c_value(ctx, value, stream);
    } else {
      write_c_expression(ctx, initialize, stream);
    }
  }
  stream_write(stream, ";");
}