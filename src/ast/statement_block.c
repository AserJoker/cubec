#include "ast/statement_block.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/statement.h"
#include "core/allocator.h"
#include "core/position.h"

ast_node_t read_ast_statement_block(allocator_t allocator, position_t *position,
                                    const char *end, const char *filename) {
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_STATEMENT_BLOCK);
  ast_node_t err = NULL;
  position_t current = *position;
  if (*current.offset != '{') {
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    goto onerror;
  }
  ast_node_t statements = create_ast_node(allocator, NODE_TYPE_LIST);
  ast_add_child(allocator, node, "statements", statements);
  for (;;) {
    ast_node_t sts = read_ast_statement(allocator, &current, end, filename);
    if (!sts) {
      break;
    }
    if (sts->type == NODE_TYPE_ERROR) {
      err = sts;
      goto onerror;
    }
    ast_add_item(statements, sts);
    err = ast_skip_all(allocator, &current, end, filename);
    if (err && err->type == NODE_TYPE_ERROR) {
      goto onerror;
    }
  }
  if (*current.offset != '}') {
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid statement, missing '}'");
    goto onerror;
  }
  current.offset++;
  current.column++;
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;

  return node;
onerror:
  allocator_free(allocator, node);
  return err;
}