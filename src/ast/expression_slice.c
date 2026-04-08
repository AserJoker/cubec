#include "ast/expression_slice.h"
#include "ast/expression.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"

cubec_ast_node_t cubec_read_ast_expression_slice(cubec_allocator_t allocator,
                                                 cubec_position_t *position,
                                                 const char *end,
                                                 const char *filename) {
  cubec_ast_node_t node =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_EXPRESSION_SLICE);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  if (*current.offset != '[') {
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  cubec_ast_node_t start =
      cubec_read_ast_expression3(allocator, &current, end, filename);
  if (start) {
    if (start->type == CUBEC_NODE_TYPE_ERROR) {
      err = start;
      goto onerror;
    }
    cubec_ast_add_child(allocator, node, "start", start);
    err = cubec_ast_skip_all(allocator, &current, end, filename);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      goto onerror;
    }
  }
  if (*current.offset != ':') {
    goto onerror;
  }
  current.offset++;
  current.column++;
  cubec_ast_node_t end_index =
      cubec_read_ast_expression3(allocator, &current, end, filename);
  if (end_index) {
    if (end_index->type == CUBEC_NODE_TYPE_ERROR) {
      err = end_index;
      goto onerror;
    }
    cubec_ast_add_child(allocator, node, "end", end_index);
  }
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  if (*current.offset != ']') {
    err = cubec_create_ast_error(allocator, *position, current, filename,
                                 "Invalid slice expression, missing ']'");
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
  cubec_allocator_free(allocator, node);
  return err;
}