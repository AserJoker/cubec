#include "fmt/statement_test.h"
#include "ast/node.h"
#include "core/stream.h"
#include "fmt/statement_block.h"

void fmt_statement_test(allocator_t allocator, ast_node_t node,
                        stream_t stream) {
  ast_node_t name = ast_get_child(node, "name");
  ast_node_t body = ast_get_child(node, "body");
  stream_write(stream, "test ");
  stream_write_location(stream, name->loc);
  stream_write(stream, " ");
  fmt_statement_block(allocator, body, stream);
}