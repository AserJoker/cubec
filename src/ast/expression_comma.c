#include "ast/expression_comma.h"
#include "ast/expression.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"

cubec_ast_node_t cubec_read_ast_expression_comma(cubec_allocator_t allocator,
                                                 cubec_position_t *position,
                                                 const char *end,
                                                 const char *filename) {
  cubec_position_t current = *position;
  cubec_ast_node_t node = NULL;
  cubec_ast_node_t err = NULL;
  node = cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_EXPRESSION_COMMON);
  cubec_ast_node_t curr =
      cubec_read_ast_expression2(allocator, &current, end, filename);
  if (!curr) {
    goto onerror;
  }
  if (curr->type == CUBEC_NODE_TYPE_ERROR) {
    err = curr;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "current", curr);
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  cubec_allocator_free(allocator, err);
  if (*current.offset != ',') {
    err = cubec_hash_map_move(node->children, "current", NULL, NULL);
    *position = err->loc.end;
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  cubec_allocator_free(allocator, err);
  cubec_ast_node_t next =
      cubec_read_ast_expression1(allocator, &current, end, filename);
  if (!next) {
    err = cubec_create_ast_error(allocator, *position, current, filename,
                                 "invalid comma expression");
    goto onerror;
  }
  if (next->type == CUBEC_NODE_TYPE_ERROR) {
    err = next;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "next", next);
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;
  return node;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}