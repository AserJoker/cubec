#include "ast/expression_slice.h"
#include "ast/expression.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"

ast_node_t read_ast_expression_slice(allocator_t allocator,
                                     position_t *position, const char *end,
                                     const char *filename) {
  ast_node_t node = NULL;
  ast_node_t err = NULL;
  position_t current = *position;
  if (*current.offset != '[') {
    goto onerror;
  }
  current.offset++;
  current.column++;
  node = create_ast_node(allocator, CUBEC_NODE_TYPE_EXPRESSION_SLICE);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  ast_node_t start = read_ast_expression3(allocator, &current, end, filename);
  if (start) {
    if (start->type == CUBEC_NODE_TYPE_ERROR) {
      err = start;
      goto onerror;
    }
    ast_add_child(allocator, node, "start", start);
    err = ast_skip_all(allocator, &current, end, filename);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      goto onerror;
    }
  }
  if (*current.offset != ':') {
    goto onerror;
  }
  current.offset++;
  current.column++;
  ast_node_t end_index =
      read_ast_expression3(allocator, &current, end, filename);
  if (end_index) {
    if (end_index->type == CUBEC_NODE_TYPE_ERROR) {
      err = end_index;
      goto onerror;
    }
    ast_add_child(allocator, node, "end", end_index);
  }
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  if (*current.offset != ']') {
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid slice expression, missing ']'");
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