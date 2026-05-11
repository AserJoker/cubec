#include "c/expression_member.h"
#include "ast/node.h"
#include "c/expression.h"
#include "core/stream.h"
#include "engine/type.h"
#include "engine/value.h"
#include "resolve/expression.h"
void write_c_expression_member(context_t ctx, ast_node_t node,
                               stream_t stream) {
  ast_node_t host = ast_get_child(node, "host");
  ast_node_t field = ast_get_child(node, "field");
  value_t vhost = resolve_expression(ctx, host);
  type_t type = value_get_type(vhost);
  if (type_get_kind(type) == TYPE_KIND_TYPE) {
    stream_write(stream, type_get_id(type));
    stream_write(stream, "V");
    stream_write_location(stream, field->loc);
  } else if (type_get_kind(type) == TYPE_KIND_PTR) {
    write_c_expression(ctx, host, stream);
    stream_write(stream, "->");
    stream_write_location(stream, field->loc);
  } else {
    write_c_expression(ctx, host, stream);
    stream_write(stream, ".");
    stream_write_location(stream, field->loc);
  }
}