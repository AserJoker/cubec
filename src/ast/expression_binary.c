#include "ast/expression_binary.h"
#include "ast/expression.h"
#include "ast/literal_symbol.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"
ast_node_t read_ast_expression_binary_logical_or(allocator_t allocator,
                                                 position_t *position,
                                                 const char *end,
                                                 const char *filename) {
  ast_node_t err = NULL;
  ast_node_t node = NULL;
  position_t current = *position;
  ast_node_t left = read_ast_expression_binary_logical_and(allocator, &current,
                                                           end, filename);
  if (!left) {
    goto onerror;
  }
  if (left->type == NODE_TYPE_ERROR) {
    err = left;
    goto onerror;
  }
  node = create_ast_node(allocator, NODE_TYPE_EXPRESSION_BINARY);
  ast_add_child(allocator, node, "left", left);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  ast_node_t opt = read_ast_literal_symbol(allocator, &current, end, filename);
  if (!opt) {
    err = hash_map_move(node->children, "left", NULL, NULL);
    *position = err->loc.end;
    goto onerror;
  }
  if (opt->type == NODE_TYPE_ERROR) {
    err = opt;
    goto onerror;
  }
  ast_add_child(allocator, node, "opt", opt);
  if (!location_is(opt->loc, "||") && !location_is(opt->loc, "??")) {
    err = hash_map_move(node->children, "left", NULL, NULL);
    *position = err->loc.end;
    goto onerror;
  }
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  ast_node_t right =
      read_ast_expression_binary_logical_or(allocator, &current, end, filename);
  if (!right) {
    err = create_ast_error(allocator, *position, current, filename,
                           "Unexpected expression");
    goto onerror;
  }
  if (right->type == NODE_TYPE_ERROR) {
    err = right;
    goto onerror;
  }
  ast_add_child(allocator, node, "right", right);
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;
  return node;
onerror:
  allocator_free(allocator, node);
  return err;
}

ast_node_t read_ast_expression_binary_logical_and(allocator_t allocator,
                                                  position_t *position,
                                                  const char *end,
                                                  const char *filename) {
  ast_node_t err = NULL;
  ast_node_t node = NULL;
  position_t current = *position;
  ast_node_t left =
      read_ast_expression_binary_bitwise_or(allocator, &current, end, filename);
  if (!left) {
    goto onerror;
  }
  if (left->type == NODE_TYPE_ERROR) {
    err = left;
    goto onerror;
  }
  node = create_ast_node(allocator, NODE_TYPE_EXPRESSION_BINARY);
  ast_add_child(allocator, node, "left", left);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  allocator_free(allocator, err);
  ast_node_t opt = read_ast_literal_symbol(allocator, &current, end, filename);
  if (!opt) {
    err = hash_map_move(node->children, "left", NULL, NULL);
    *position = err->loc.end;
    goto onerror;
  }
  if (opt->type == NODE_TYPE_ERROR) {
    err = opt;
    goto onerror;
  }
  ast_add_child(allocator, node, "opt", opt);
  if (!location_is(opt->loc, "&&")) {
    err = hash_map_move(node->children, "left", NULL, NULL);
    *position = err->loc.end;
    goto onerror;
  }
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  ast_node_t right = read_ast_expression_binary_logical_and(allocator, &current,
                                                            end, filename);
  if (!right) {
    err = create_ast_error(allocator, *position, current, filename,
                           "Unexpected expression");
    goto onerror;
  }
  if (right->type == NODE_TYPE_ERROR) {
    err = right;
    goto onerror;
  }
  ast_add_child(allocator, node, "right", right);
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;
  return node;
onerror:
  allocator_free(allocator, node);
  return err;
}

ast_node_t read_ast_expression_binary_bitwise_or(allocator_t allocator,
                                                 position_t *position,
                                                 const char *end,
                                                 const char *filename) {
  ast_node_t err = NULL;
  ast_node_t node = NULL;
  position_t current = *position;
  ast_node_t left = read_ast_expression_binary_bitwise_xor(allocator, &current,
                                                           end, filename);
  if (!left) {
    goto onerror;
  }
  if (left->type == NODE_TYPE_ERROR) {
    err = left;
    goto onerror;
  }
  node = create_ast_node(allocator, NODE_TYPE_EXPRESSION_BINARY);
  ast_add_child(allocator, node, "left", left);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  ast_node_t opt = read_ast_literal_symbol(allocator, &current, end, filename);
  if (!opt) {
    err = hash_map_move(node->children, "left", NULL, NULL);
    *position = err->loc.end;
    goto onerror;
  }
  if (opt->type == NODE_TYPE_ERROR) {
    err = opt;
    goto onerror;
  }
  ast_add_child(allocator, node, "opt", opt);
  if (!location_is(opt->loc, "|")) {
    err = hash_map_move(node->children, "left", NULL, NULL);
    *position = err->loc.end;
    goto onerror;
  }
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  ast_node_t right =
      read_ast_expression_binary_bitwise_or(allocator, &current, end, filename);
  if (!right) {
    err = create_ast_error(allocator, *position, current, filename,
                           "Unexpected expression");
    goto onerror;
  }
  if (right->type == NODE_TYPE_ERROR) {
    err = right;
    goto onerror;
  }
  ast_add_child(allocator, node, "right", right);
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;

  return node;
onerror:
  allocator_free(allocator, node);
  return err;
}
ast_node_t read_ast_expression_binary_bitwise_xor(allocator_t allocator,
                                                  position_t *position,
                                                  const char *end,
                                                  const char *filename) {
  ast_node_t err = NULL;
  ast_node_t node = NULL;
  position_t current = *position;
  ast_node_t left = read_ast_expression_binary_bitwise_and(allocator, &current,
                                                           end, filename);
  if (!left) {
    goto onerror;
  }
  if (left->type == NODE_TYPE_ERROR) {
    err = left;
    goto onerror;
  }
  node = create_ast_node(allocator, NODE_TYPE_EXPRESSION_BINARY);
  ast_add_child(allocator, node, "left", left);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  ast_node_t opt = read_ast_literal_symbol(allocator, &current, end, filename);
  if (!opt) {
    err = hash_map_move(node->children, "left", NULL, NULL);
    *position = err->loc.end;
    goto onerror;
  }
  if (opt->type == NODE_TYPE_ERROR) {
    err = opt;
    goto onerror;
  }
  ast_add_child(allocator, node, "opt", opt);
  if (!location_is(opt->loc, "^")) {
    err = hash_map_move(node->children, "left", NULL, NULL);
    *position = err->loc.end;
    goto onerror;
  }
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  allocator_free(allocator, err);
  ast_node_t right = read_ast_expression_binary_bitwise_xor(allocator, &current,
                                                            end, filename);
  if (!right) {
    err = create_ast_error(allocator, *position, current, filename,
                           "Unexpected expression");
    goto onerror;
  }
  if (right->type == NODE_TYPE_ERROR) {
    err = right;
    goto onerror;
  }
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;

  return node;
onerror:
  allocator_free(allocator, node);
  return err;
}
ast_node_t read_ast_expression_binary_bitwise_and(allocator_t allocator,
                                                  position_t *position,
                                                  const char *end,
                                                  const char *filename) {
  ast_node_t err = NULL;
  ast_node_t node = NULL;
  position_t current = *position;
  ast_node_t left =
      read_ast_expression_binary_equal(allocator, &current, end, filename);
  if (!left) {
    goto onerror;
  }
  if (left->type == NODE_TYPE_ERROR) {
    err = left;
    goto onerror;
  }
  node = create_ast_node(allocator, NODE_TYPE_EXPRESSION_BINARY);
  ast_add_child(allocator, node, "left", left);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  allocator_free(allocator, err);
  ast_node_t opt = read_ast_literal_symbol(allocator, &current, end, filename);
  if (!opt) {
    err = hash_map_move(node->children, "left", NULL, NULL);
    *position = err->loc.end;
    goto onerror;
  }
  if (opt->type == NODE_TYPE_ERROR) {
    err = opt;
    goto onerror;
  }
  ast_add_child(allocator, node, "opt", opt);
  if (!location_is(opt->loc, "&")) {
    err = hash_map_move(node->children, "left", NULL, NULL);
    *position = err->loc.end;
    goto onerror;
  }
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  allocator_free(allocator, err);
  ast_node_t right = read_ast_expression_binary_bitwise_and(allocator, &current,
                                                            end, filename);
  if (!right) {
    err = create_ast_error(allocator, *position, current, filename,
                           "Unexpected expression");
    goto onerror;
  }
  if (right->type == NODE_TYPE_ERROR) {
    err = right;
    goto onerror;
  }
  ast_add_child(allocator, node, "right", right);
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;
  return node;
onerror:
  allocator_free(allocator, node);
  return err;
}
ast_node_t read_ast_expression_binary_equal(allocator_t allocator,
                                            position_t *position,
                                            const char *end,
                                            const char *filename) {

  ast_node_t err = NULL;
  ast_node_t node = NULL;
  position_t current = *position;
  ast_node_t left =
      read_ast_expression_binary_relation(allocator, &current, end, filename);
  if (!left) {
    goto onerror;
  }
  if (left->type == NODE_TYPE_ERROR) {
    err = left;
    goto onerror;
  }
  node = create_ast_node(allocator, NODE_TYPE_EXPRESSION_BINARY);
  ast_add_child(allocator, node, "left", left);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  allocator_free(allocator, err);
  ast_node_t opt = read_ast_literal_symbol(allocator, &current, end, filename);
  if (!opt) {
    err = hash_map_move(node->children, "left", NULL, NULL);
    *position = err->loc.end;
    goto onerror;
  }
  if (opt->type == NODE_TYPE_ERROR) {
    err = opt;
    goto onerror;
  }
  ast_add_child(allocator, node, "opt", opt);
  if (!location_is(opt->loc, "==") && !location_is(opt->loc, "!=")) {
    err = hash_map_move(node->children, "left", NULL, NULL);
    *position = err->loc.end;
    goto onerror;
  }
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  allocator_free(allocator, err);
  ast_node_t right =
      read_ast_expression_binary_equal(allocator, &current, end, filename);
  if (!right) {
    err = create_ast_error(allocator, *position, current, filename,
                           "Unexpected expression");
    goto onerror;
  }
  if (right->type == NODE_TYPE_ERROR) {
    err = right;
    goto onerror;
  }
  ast_add_child(allocator, node, "right", right);
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;

  return node;
onerror:
  allocator_free(allocator, node);
  return err;
}
ast_node_t read_ast_expression_binary_relation(allocator_t allocator,
                                               position_t *position,
                                               const char *end,
                                               const char *filename) {

  ast_node_t err = NULL;
  ast_node_t node = NULL;
  position_t current = *position;
  ast_node_t left = read_ast_expression_binary_bitwise_shift(
      allocator, &current, end, filename);
  if (!left) {
    goto onerror;
  }
  if (left->type == NODE_TYPE_ERROR) {
    err = left;
    goto onerror;
  }
  node = create_ast_node(allocator, NODE_TYPE_EXPRESSION_BINARY);
  ast_add_child(allocator, node, "left", left);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  allocator_free(allocator, err);
  ast_node_t opt = read_ast_literal_symbol(allocator, &current, end, filename);
  if (!opt) {
    err = hash_map_move(node->children, "left", NULL, NULL);
    *position = err->loc.end;
    goto onerror;
  }
  if (opt->type == NODE_TYPE_ERROR) {
    err = opt;
    goto onerror;
  }
  ast_add_child(allocator, node, "opt", opt);
  if (!location_is(opt->loc, ">") && !location_is(opt->loc, "<") &&
      !location_is(opt->loc, ">=") && !location_is(opt->loc, "<=")) {
    err = hash_map_move(node->children, "left", NULL, NULL);
    *position = err->loc.end;
    goto onerror;
  }
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  allocator_free(allocator, err);
  ast_node_t right =
      read_ast_expression_binary_relation(allocator, &current, end, filename);
  if (!right) {
    err = create_ast_error(allocator, *position, current, filename,
                           "Unexpected expression");
    goto onerror;
  }
  if (right->type == NODE_TYPE_ERROR) {
    err = right;
    goto onerror;
  }
  ast_add_child(allocator, node, "right", right);
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;

  return node;
onerror:
  allocator_free(allocator, node);
  return err;
}
ast_node_t read_ast_expression_binary_bitwise_shift(allocator_t allocator,
                                                    position_t *position,
                                                    const char *end,
                                                    const char *filename) {

  ast_node_t err = NULL;
  ast_node_t node = NULL;
  position_t current = *position;
  ast_node_t left =
      read_ast_expression_binary_additive(allocator, &current, end, filename);
  if (!left) {
    goto onerror;
  }
  if (left->type == NODE_TYPE_ERROR) {
    err = left;
    goto onerror;
  }
  node = create_ast_node(allocator, NODE_TYPE_EXPRESSION_BINARY);
  ast_add_child(allocator, node, "left", left);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  ast_node_t opt = read_ast_literal_symbol(allocator, &current, end, filename);
  if (!opt) {
    err = hash_map_move(node->children, "left", NULL, NULL);
    *position = err->loc.end;
    goto onerror;
  }
  if (opt->type == NODE_TYPE_ERROR) {
    err = opt;
    goto onerror;
  }
  ast_add_child(allocator, node, "opt", opt);
  if (!location_is(opt->loc, ">>") && !location_is(opt->loc, "<<")) {
    err = hash_map_move(node->children, "left", NULL, NULL);
    *position = err->loc.end;
    goto onerror;
  }
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  ast_node_t right = read_ast_expression_binary_bitwise_shift(
      allocator, &current, end, filename);
  if (!right) {
    err = create_ast_error(allocator, *position, current, filename,
                           "Unexpected expression");
    goto onerror;
  }
  if (right->type == NODE_TYPE_ERROR) {
    err = right;
    goto onerror;
  }
  ast_add_child(allocator, node, "right", right);
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;

  return node;
onerror:
  allocator_free(allocator, node);
  return err;
}
ast_node_t read_ast_expression_binary_additive(allocator_t allocator,
                                               position_t *position,
                                               const char *end,
                                               const char *filename) {
  ast_node_t err = NULL;
  ast_node_t node = NULL;
  position_t current = *position;
  ast_node_t left = read_ast_expression_binary_multiplicative(
      allocator, &current, end, filename);
  if (!left) {
    goto onerror;
  }
  if (left->type == NODE_TYPE_ERROR) {
    err = left;
    goto onerror;
  }
  node = create_ast_node(allocator, NODE_TYPE_EXPRESSION_BINARY);
  ast_add_child(allocator, node, "left", left);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  ast_node_t opt = read_ast_literal_symbol(allocator, &current, end, filename);
  if (!opt) {
    err = hash_map_move(node->children, "left", NULL, NULL);
    *position = err->loc.end;
    goto onerror;
  }
  if (opt->type == NODE_TYPE_ERROR) {
    err = opt;
    goto onerror;
  }
  ast_add_child(allocator, node, "opt", opt);
  if (!location_is(opt->loc, "+") && !location_is(opt->loc, "-")) {
    err = hash_map_move(node->children, "left", NULL, NULL);
    *position = err->loc.end;
    goto onerror;
  }
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  allocator_free(allocator, err);
  ast_node_t right =
      read_ast_expression_binary_additive(allocator, &current, end, filename);
  if (!right) {
    err = create_ast_error(allocator, *position, current, filename,
                           "Unexpected expression");
    goto onerror;
  }
  if (right->type == NODE_TYPE_ERROR) {
    err = right;
    goto onerror;
  }
  ast_add_child(allocator, node, "right", right);
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;
  return node;
onerror:
  allocator_free(allocator, node);
  return err;
}
ast_node_t read_ast_expression_binary_multiplicative(allocator_t allocator,
                                                     position_t *position,
                                                     const char *end,
                                                     const char *filename) {
  ast_node_t err = NULL;
  ast_node_t node = NULL;
  position_t current = *position;
  ast_node_t left =
      read_ast_expression_binary_prefix(allocator, &current, end, filename);
  if (!left) {
    goto onerror;
  }
  if (left->type == NODE_TYPE_ERROR) {
    err = left;
    goto onerror;
  }
  node = create_ast_node(allocator, NODE_TYPE_EXPRESSION_BINARY);
  ast_add_child(allocator, node, "left", left);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  ast_node_t opt = read_ast_literal_symbol(allocator, &current, end, filename);
  if (!opt) {
    err = hash_map_move(node->children, "left", NULL, NULL);
    *position = err->loc.end;
    goto onerror;
  }
  if (opt->type == NODE_TYPE_ERROR) {
    err = opt;
    goto onerror;
  }
  ast_add_child(allocator, node, "opt", opt);
  if (!location_is(opt->loc, "*") && !location_is(opt->loc, "/") &&
      !location_is(opt->loc, "%")) {
    err = hash_map_move(node->children, "left", NULL, NULL);
    *position = err->loc.end;
    goto onerror;
  }
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  allocator_free(allocator, err);
  ast_node_t right = read_ast_expression_binary_multiplicative(
      allocator, &current, end, filename);
  if (!right) {
    err = create_ast_error(allocator, *position, current, filename,
                           "Unexpected expression");
    goto onerror;
  }
  if (right->type == NODE_TYPE_ERROR) {
    err = right;
    goto onerror;
  }
  ast_add_child(allocator, node, "right", right);
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;
  return node;
onerror:
  allocator_free(allocator, node);
  return err;
}
ast_node_t read_ast_expression_binary_prefix(allocator_t allocator,
                                             position_t *position,
                                             const char *end,
                                             const char *filename) {
  ast_node_t err = NULL;
  ast_node_t node = NULL;
  position_t current = *position;
  ast_node_t opt = NULL;
  if (*current.offset == '&') {
    opt = create_ast_node(allocator, NODE_TYPE_LITERAL_SYMBOL);
    opt->loc.begin = current;
    current.offset++;
    current.column++;
    opt->loc.end = current;
  } else {
    opt = read_ast_literal_symbol(allocator, &current, end, filename);
    if (!opt) {
      err = read_ast_expression_value(allocator, &current, end, filename);
      goto onerror;
    }
    if (opt->type == NODE_TYPE_ERROR) {
      err = opt;
      goto onerror;
    }
  }
  node = create_ast_node(allocator, NODE_TYPE_EXPRESSION_BINARY);
  ast_add_child(allocator, node, "opt", opt);
  if (!location_is(opt->loc, "!") && !location_is(opt->loc, "+") &&
      !location_is(opt->loc, "-") && !location_is(opt->loc, "~") &&
      !location_is(opt->loc, "&") && !location_is(opt->loc, "*")) {
    current = opt->loc.begin;
    err = read_ast_expression_value(allocator, &current, end, filename);
    goto onerror;
  }

  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  ast_node_t right =
      read_ast_expression_binary_prefix(allocator, &current, end, filename);
  if (!right) {
    err = create_ast_error(allocator, *position, current, filename,
                           "Unexpected expression");
    goto onerror;
  }
  if (right->type == NODE_TYPE_ERROR) {
    err = right;
    goto onerror;
  }
  ast_add_child(allocator, node, "right", right);
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;
  return node;
onerror:
  allocator_free(allocator, node);
  return err;
}