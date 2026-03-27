#include "ast/statement_block.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/statement.h"
#include "core/allocator.h"
#include "core/position.h"

cubec_ast_node_t cubec_read_ast_statement_block(cubec_allocator_t allocator,
                                                cubec_position_t *position,
                                                const char *end) {
  cubec_ast_node_t node =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_STATEMENT_BLOCK);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  if (*current.offset != '{') {
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  cubec_ast_node_t statements =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_LIST);
  cubec_ast_add_child(allocator, node, "statements", statements);
  for (;;) {
    cubec_ast_node_t sts = cubec_read_ast_statement(allocator, &current, end);
    if (!sts) {
      break;
    }
    if (sts->type == CUBEC_NODE_TYPE_ERROR) {
      err = sts;
      goto onerror;
    }
    cubec_ast_add_item(allocator, statements, sts);
    err = cubec_ast_skip_all(allocator, &current, end);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      goto onerror;
    }
  }
  if (*current.offset != '}') {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid statement, missing '}'");
    goto onerror;
  }
  current.offset++;
  current.column++;
  node->loc.begin = *position;
  node->loc.end = current;
  *position = current;

  return node;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}