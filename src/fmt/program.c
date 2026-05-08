#include "fmt/program.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/stream.h"
#include "fmt/statement_declaration.h"
#include "fmt/statement_function.h"
#include "fmt/statement_struct.h"
#include "fmt/statement_test.h"

void fmt_program(allocator_t allocator, ast_node_t node, stream_t stream) {
  ast_node_t statements = ast_get_child(node, "statements");
  size_t count = 0;
  for (size_t idx = 0; idx < ast_get_length(statements); idx++) {
    ast_node_t sts = ast_get_item(statements, idx);
    if (count != 0) {
      stream_newline(stream);
    }
    if (sts->visible) {
      if (sts->type == NODE_TYPE_STATEMENT_DECLARATION) {
        fmt_statement_declaration(allocator, sts, stream);
      } else if (sts->type == NODE_TYPE_STATEMENT_FUNCTION) {
        fmt_statement_function(allocator, sts, stream);
      } else if (sts->type == NODE_TYPE_STATEMENT_STRUCT) {
        fmt_statement_struct(allocator, sts, stream);
      } else if (sts->type == NODE_TYPE_STATEMENT_TEST) {
        fmt_statement_test(allocator, sts, stream);
      }
      count++;
    }
  }
}