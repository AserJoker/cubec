#include "writer/ptr_declarator.h"
#include "ast/node.h"
#include "core/stream.h"
#include "writer/expression.h"
void write_ptr_declarator(allocator_t allocator, ast_node_t node,
                          stream_t stream) {
  ast_node_t kind = ast_get_child(node, "kind");
  ast_node_t decorators = ast_get_child(node, "decorators");
  ast_node_t type = ast_get_child(node, "type");
  stream_write_location(stream, kind->loc);
  for (size_t idx = 0; idx < ast_get_length(decorators); idx++) {
    ast_node_t dec = ast_get_item(decorators, idx);
    stream_write_location(stream, dec->loc);
    stream_write(stream, " ");
  }
  write_expression(allocator, type, stream);
}