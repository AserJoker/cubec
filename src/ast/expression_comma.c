#include "ast/expression_comma.h"
#include "ast/expression_assigment.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"

ast_node_t read_ast_expression_comma(allocator_t allocator,
                                     position_t *position, const char *end,
                                     const char *filename) {
  position_t current = *position;
  ast_node_t node = NULL;
  ast_node_t err = NULL;
  node = create_ast_node(allocator, NODE_TYPE_EXPRESSION_COMMON);
  ast_node_t curr =
      read_ast_expression_assigment(allocator, &current, end, filename);
  if (!curr) {
    goto onerror;
  }
  if (curr->type == NODE_TYPE_ERROR) {
    err = curr;
    goto onerror;
  }
  ast_add_child(allocator, node, "current", curr);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    goto onerror;
  }
  allocator_free(allocator, err);
  if (*current.offset != ',') {
    err = hash_map_move(node->children, "current", NULL, NULL);
    *position = err->loc.end;
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    goto onerror;
  }
  allocator_free(allocator, err);
  ast_node_t next =
      read_ast_expression_assigment(allocator, &current, end, filename);
  if (!next) {
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid comma expression");
    goto onerror;
  }
  if (next->type == NODE_TYPE_ERROR) {
    err = next;
    goto onerror;
  }
  ast_add_child(allocator, node, "next", next);
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;
  return node;
onerror:
  allocator_free(allocator, node);
  return err;
}