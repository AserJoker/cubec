#include "ast/expression_condition.h"
#include "ast/expression.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"

ast_node_t read_ast_expression_condition(allocator_t allocator,
                                         position_t *position, const char *end,
                                         const char *filename) {
  ast_node_t node = NULL;
  ast_node_t err = NULL;
  position_t current = *position;

  ast_node_t condition =
      read_ast_expression4(allocator, &current, end, filename);
  if (!condition) {
    goto onerror;
  }
  if (condition->type == CUBEC_NODE_TYPE_ERROR) {
    err = condition;
    goto onerror;
  }
  node = create_ast_node(allocator, CUBEC_NODE_TYPE_EXPRESSION_CONDITION);
  ast_add_child(allocator, node, "condition", condition);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  if (*current.offset != '?') {
    err = hash_map_move(node->children, "condition", NULL, NULL);
    *position = err->loc.end;
    goto onerror;
  }
  current.column++;
  current.offset++;
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  ast_node_t consequent =
      read_ast_expression3(allocator, &current, end, filename);
  if (!consequent) {
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid or unexpected token");
    goto onerror;
  }
  if (consequent->type == CUBEC_NODE_TYPE_ERROR) {
    err = consequent;
    goto onerror;
  }
  ast_add_child(allocator, node, "consequent", consequent);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  if (*current.offset != ':') {
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid or unexpected token");
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  ast_node_t alternate =
      read_ast_expression3(allocator, &current, end, filename);
  if (!alternate) {
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid or unexpected token");
    goto onerror;
  }
  if (alternate->type == CUBEC_NODE_TYPE_ERROR) {
    err = alternate;
    goto onerror;
  }
  ast_add_child(allocator, node, "alternate", alternate);
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;

  return node;
onerror:
  allocator_free(allocator, node);
  return err;
}