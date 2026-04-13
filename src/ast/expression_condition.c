#include "ast/expression_condition.h"
#include "ast/expression.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"

cubec_ast_node_t
cubec_read_ast_expression_condition(cubec_allocator_t allocator,
                                    cubec_position_t *position, const char *end,
                                    const char *filename) {
  cubec_ast_node_t node =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_EXPRESSION_CONDITION);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;

  cubec_ast_node_t condition =
      cubec_read_ast_expression4(allocator, &current, end, filename);
  if (!condition) {
    goto onerror;
  }
  if (condition->type == CUBEC_NODE_TYPE_ERROR) {
    err = condition;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "condition", condition);
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  if (*current.offset != '?') {
    err = cubec_hash_map_move(node->children, "condition", NULL);
    *position = err->loc.end;
    goto onerror;
  }
  current.column++;
  current.offset++;
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  cubec_ast_node_t consequent =
      cubec_read_ast_expression3(allocator, &current, end, filename);
  if (!consequent) {
    err = cubec_create_ast_error(allocator, *position, current, filename,
                                 "Invalid or unexpected token");
    goto onerror;
  }
  if (consequent->type == CUBEC_NODE_TYPE_ERROR) {
    err = consequent;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "consequent", consequent);
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  if (*current.offset != ':') {
    err = cubec_create_ast_error(allocator, *position, current, filename,
                                 "Invalid or unexpected token");
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  cubec_ast_node_t alternate =
      cubec_read_ast_expression3(allocator, &current, end, filename);
  if (!alternate) {
    err = cubec_create_ast_error(allocator, *position, current, filename,
                                 "Invalid or unexpected token");
    goto onerror;
  }
  if (alternate->type == CUBEC_NODE_TYPE_ERROR) {
    err = alternate;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "alternate", alternate);
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;

  return node;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}