#include "ast/expression_binary.h"
#include "ast/expression.h"
#include "ast/literal_symbol.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/map.h"
#include "core/position.h"
cubec_ast_node_t cubec_read_ast_expression_binary_logical_or(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end) {
  cubec_ast_node_t err = NULL;
  cubec_ast_node_t node =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_EXPRESSION_BINARY);
  cubec_position_t current = *position;
  cubec_ast_node_t left = cubec_read_ast_expression5(allocator, &current, end);
  if (!left) {
    goto onerror;
  }
  if (left->type == CUBEC_NODE_TYPE_ERROR) {
    err = left;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "left", left);
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_ast_node_t opt =
      cubec_read_ast_literal_symbol(allocator, &current, end);
  if (!opt) {
    err = cubec_map_move(node->children, "left", NULL);
    *position = err->loc.end;
    goto onerror;
  }
  if (opt->type == CUBEC_NODE_TYPE_ERROR) {
    err = opt;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "opt", opt);
  if (!cubec_location_is(opt->loc, "||") &&
      !cubec_location_is(opt->loc, "??")) {
    err = cubec_map_move(node->children, "left", NULL);
    *position = err->loc.end;
    goto onerror;
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_ast_node_t right = cubec_read_ast_expression4(allocator, &current, end);
  if (!right) {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Unexpected expression");
    goto onerror;
  }
  if (right->type == CUBEC_NODE_TYPE_ERROR) {
    err = right;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "right", right);
  node->loc.begin = *position;
  node->loc.end = current;
  *position = current;
  return node;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}

cubec_ast_node_t cubec_read_ast_expression_binary_logical_and(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end) {
  cubec_ast_node_t err = NULL;
  cubec_ast_node_t node = NULL;
  cubec_position_t current = *position;
  node = cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_EXPRESSION_BINARY);
  cubec_ast_node_t left = cubec_read_ast_expression6(allocator, &current, end);
  if (!left) {
    goto onerror;
  }
  if (left->type == CUBEC_NODE_TYPE_ERROR) {
    err = left;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "left", left);
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_allocator_free(allocator, err);
  cubec_ast_node_t opt =
      cubec_read_ast_literal_symbol(allocator, &current, end);
  if (!opt) {
    err = cubec_map_move(node->children, "left", NULL);
    *position = err->loc.end;
    goto onerror;
  }
  if (opt->type == CUBEC_NODE_TYPE_ERROR) {
    err = opt;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "opt", opt);
  if (!cubec_location_is(opt->loc, "&&")) {
    err = cubec_map_move(node->children, "left", NULL);
    *position = err->loc.end;
    goto onerror;
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_ast_node_t right = cubec_read_ast_expression5(allocator, &current, end);
  if (!right) {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Unexpected expression");
    goto onerror;
  }
  if (right->type == CUBEC_NODE_TYPE_ERROR) {
    err = right;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "right", right);
  node->loc.begin = *position;
  node->loc.end = current;
  *position = current;
  return node;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}

cubec_ast_node_t cubec_read_ast_expression_binary_bitwise_or(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end) {
  cubec_ast_node_t err = NULL;
  cubec_ast_node_t node = NULL;
  cubec_position_t current = *position;
  node = cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_EXPRESSION_BINARY);
  cubec_ast_node_t left = cubec_read_ast_expression7(allocator, &current, end);
  if (!left) {
    goto onerror;
  }
  if (left->type == CUBEC_NODE_TYPE_ERROR) {
    err = left;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "left", left);
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_ast_node_t opt =
      cubec_read_ast_literal_symbol(allocator, &current, end);
  if (!opt) {
    err = cubec_map_move(node->children, "left", NULL);
    *position = err->loc.end;
    goto onerror;
  }
  if (opt->type == CUBEC_NODE_TYPE_ERROR) {
    err = opt;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "opt", opt);
  if (!cubec_location_is(opt->loc, "|")) {
    err = cubec_map_move(node->children, "left", NULL);
    *position = err->loc.end;
    goto onerror;
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_ast_node_t right = cubec_read_ast_expression6(allocator, &current, end);
  if (!right) {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Unexpected expression");
    goto onerror;
  }
  if (right->type == CUBEC_NODE_TYPE_ERROR) {
    err = right;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "right", right);
  node->loc.begin = *position;
  node->loc.end = current;
  *position = current;

  return node;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}
cubec_ast_node_t cubec_read_ast_expression_binary_bitwise_xor(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end) {
  cubec_ast_node_t err = NULL;
  cubec_ast_node_t node = NULL;
  cubec_position_t current = *position;
  node = cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_EXPRESSION_BINARY);
  cubec_ast_node_t left = cubec_read_ast_expression8(allocator, &current, end);
  if (!left) {
    goto onerror;
  }
  if (left->type == CUBEC_NODE_TYPE_ERROR) {
    err = left;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "left", left);
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_ast_node_t opt =
      cubec_read_ast_literal_symbol(allocator, &current, end);
  if (!opt) {
    err = cubec_map_move(node->children, "left", NULL);
    *position = err->loc.end;
    goto onerror;
  }
  if (opt->type == CUBEC_NODE_TYPE_ERROR) {
    err = opt;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "opt", opt);
  if (!cubec_location_is(opt->loc, "^")) {
    err = cubec_map_move(node->children, "left", NULL);
    *position = err->loc.end;
    goto onerror;
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_allocator_free(allocator, err);
  cubec_ast_node_t right = cubec_read_ast_expression7(allocator, &current, end);
  if (!right) {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Unexpected expression");
    goto onerror;
  }
  if (right->type == CUBEC_NODE_TYPE_ERROR) {
    err = right;
    goto onerror;
  }
  node->loc.begin = *position;
  node->loc.end = current;
  *position = current;

  return node;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}
cubec_ast_node_t cubec_read_ast_expression_binary_bitwise_and(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end) {
  cubec_ast_node_t err = NULL;
  cubec_ast_node_t node = NULL;
  cubec_position_t current = *position;
  node = cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_EXPRESSION_BINARY);
  cubec_ast_node_t left = cubec_read_ast_expression9(allocator, &current, end);
  if (!left) {
    goto onerror;
  }
  if (left->type == CUBEC_NODE_TYPE_ERROR) {
    err = left;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "left", left);
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_allocator_free(allocator, err);
  cubec_ast_node_t opt =
      cubec_read_ast_literal_symbol(allocator, &current, end);
  if (!opt) {
    err = cubec_map_move(node->children, "left", NULL);
    *position = err->loc.end;
    goto onerror;
  }
  if (opt->type == CUBEC_NODE_TYPE_ERROR) {
    err = opt;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "opt", opt);
  if (!cubec_location_is(opt->loc, "&")) {
    err = cubec_map_move(node->children, "left", NULL);
    *position = err->loc.end;
    goto onerror;
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_allocator_free(allocator, err);
  cubec_ast_node_t right = cubec_read_ast_expression8(allocator, &current, end);
  if (!right) {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Unexpected expression");
    goto onerror;
  }
  if (right->type == CUBEC_NODE_TYPE_ERROR) {
    err = right;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "right", right);
  node->loc.begin = *position;
  node->loc.end = current;
  *position = current;
  return node;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}
cubec_ast_node_t cubec_read_ast_expression_binary_equal(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end) {

  cubec_ast_node_t err = NULL;
  cubec_ast_node_t node = NULL;
  cubec_position_t current = *position;
  node = cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_EXPRESSION_BINARY);
  cubec_ast_node_t left = cubec_read_ast_expression10(allocator, &current, end);
  if (!left) {
    goto onerror;
  }
  if (left->type == CUBEC_NODE_TYPE_ERROR) {
    err = left;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "left", left);
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_allocator_free(allocator, err);
  cubec_ast_node_t opt =
      cubec_read_ast_literal_symbol(allocator, &current, end);
  if (!opt) {
    err = cubec_map_move(node->children, "left", NULL);
    *position = err->loc.end;
    goto onerror;
  }
  if (opt->type == CUBEC_NODE_TYPE_ERROR) {
    err = opt;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "opt", opt);
  if (!cubec_location_is(opt->loc, "==") &&
      !cubec_location_is(opt->loc, "!=")) {
    err = cubec_map_move(node->children, "left", NULL);
    *position = err->loc.end;
    goto onerror;
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_allocator_free(allocator, err);
  cubec_ast_node_t right = cubec_read_ast_expression9(allocator, &current, end);
  if (!right) {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Unexpected expression");
    goto onerror;
  }
  if (right->type == CUBEC_NODE_TYPE_ERROR) {
    err = right;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "right", right);
  node->loc.begin = *position;
  node->loc.end = current;
  *position = current;

  return node;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}
cubec_ast_node_t cubec_read_ast_expression_binary_relation(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end) {

  cubec_ast_node_t err = NULL;
  cubec_ast_node_t node = NULL;
  cubec_position_t current = *position;
  node = cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_EXPRESSION_BINARY);
  cubec_ast_node_t left = cubec_read_ast_expression11(allocator, &current, end);
  if (!left) {
    goto onerror;
  }
  if (left->type == CUBEC_NODE_TYPE_ERROR) {
    err = left;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "left", left);
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_allocator_free(allocator, err);
  cubec_ast_node_t opt =
      cubec_read_ast_literal_symbol(allocator, &current, end);
  if (!opt) {
    err = cubec_map_move(node->children, "left", NULL);
    *position = err->loc.end;
    goto onerror;
  }
  if (opt->type == CUBEC_NODE_TYPE_ERROR) {
    err = opt;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "opt", opt);
  if (!cubec_location_is(opt->loc, ">") && !cubec_location_is(opt->loc, "<") &&
      !cubec_location_is(opt->loc, ">=") &&
      !cubec_location_is(opt->loc, "<=")) {
    err = cubec_map_move(node->children, "left", NULL);
    *position = err->loc.end;
    goto onerror;
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_allocator_free(allocator, err);
  cubec_ast_node_t right =
      cubec_read_ast_expression10(allocator, &current, end);
  if (!right) {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Unexpected expression");
    goto onerror;
  }
  if (right->type == CUBEC_NODE_TYPE_ERROR) {
    err = right;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "right", right);
  node->loc.begin = *position;
  node->loc.end = current;
  *position = current;

  return node;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}
cubec_ast_node_t cubec_read_ast_expression_binary_bitwise_shift(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end) {

  cubec_ast_node_t err = NULL;
  cubec_ast_node_t node = NULL;
  cubec_position_t current = *position;
  node = cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_EXPRESSION_BINARY);
  cubec_ast_node_t left = cubec_read_ast_expression12(allocator, &current, end);
  if (!left) {
    goto onerror;
  }
  if (left->type == CUBEC_NODE_TYPE_ERROR) {
    err = left;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "left", left);
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_ast_node_t opt =
      cubec_read_ast_literal_symbol(allocator, &current, end);
  if (!opt) {
    err = cubec_map_move(node->children, "left", NULL);
    *position = err->loc.end;
    goto onerror;
  }
  if (opt->type == CUBEC_NODE_TYPE_ERROR) {
    err = opt;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "opt", opt);
  if (!cubec_location_is(opt->loc, ">>") &&
      !cubec_location_is(opt->loc, "<<")) {
    err = cubec_map_move(node->children, "left", NULL);
    *position = err->loc.end;
    goto onerror;
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_ast_node_t right =
      cubec_read_ast_expression11(allocator, &current, end);
  if (!right) {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Unexpected expression");
    goto onerror;
  }
  if (right->type == CUBEC_NODE_TYPE_ERROR) {
    err = right;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "right", right);
  node->loc.begin = *position;
  node->loc.end = current;
  *position = current;

  return node;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}
cubec_ast_node_t cubec_read_ast_expression_binary_additive(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end) {
  cubec_ast_node_t err = NULL;
  cubec_ast_node_t node = NULL;
  cubec_position_t current = *position;
  node = cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_EXPRESSION_BINARY);
  cubec_ast_node_t left = cubec_read_ast_expression13(allocator, &current, end);
  if (!left) {
    goto onerror;
  }
  if (left->type == CUBEC_NODE_TYPE_ERROR) {
    err = left;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "left", left);
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_ast_node_t opt =
      cubec_read_ast_literal_symbol(allocator, &current, end);
  if (!opt) {
    err = cubec_map_move(node->children, "left", NULL);
    *position = err->loc.end;
    goto onerror;
  }
  if (opt->type == CUBEC_NODE_TYPE_ERROR) {
    err = opt;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "opt", opt);
  if (!cubec_location_is(opt->loc, "+") && !cubec_location_is(opt->loc, "-")) {
    err = cubec_map_move(node->children, "left", NULL);
    *position = err->loc.end;
    goto onerror;
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_allocator_free(allocator, err);
  cubec_ast_node_t right =
      cubec_read_ast_expression12(allocator, &current, end);
  if (!right) {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Unexpected expression");
    goto onerror;
  }
  if (right->type == CUBEC_NODE_TYPE_ERROR) {
    err = right;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "right", right);
  node->loc.begin = *position;
  node->loc.end = current;
  *position = current;
  return node;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}
cubec_ast_node_t cubec_read_ast_expression_binary_multiplicative(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end) {
  cubec_ast_node_t err = NULL;
  cubec_ast_node_t node = NULL;
  cubec_position_t current = *position;
  node = cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_EXPRESSION_BINARY);
  cubec_ast_node_t left = cubec_read_ast_expression14(allocator, &current, end);
  if (!left) {
    goto onerror;
  }
  if (left->type == CUBEC_NODE_TYPE_ERROR) {
    err = left;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "left", left);
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_ast_node_t opt =
      cubec_read_ast_literal_symbol(allocator, &current, end);
  if (!opt) {
    err = cubec_map_move(node->children, "left", NULL);
    *position = err->loc.end;
    goto onerror;
  }
  if (opt->type == CUBEC_NODE_TYPE_ERROR) {
    err = opt;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "opt", opt);
  if (!cubec_location_is(opt->loc, "*") && !cubec_location_is(opt->loc, "/") &&
      !cubec_location_is(opt->loc, "%")) {
    err = cubec_map_move(node->children, "left", NULL);
    *position = err->loc.end;
    goto onerror;
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_allocator_free(allocator, err);
  cubec_ast_node_t right =
      cubec_read_ast_expression13(allocator, &current, end);
  if (!right) {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Unexpected expression");
    goto onerror;
  }
  if (right->type == CUBEC_NODE_TYPE_ERROR) {
    err = right;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "right", right);
  node->loc.begin = *position;
  node->loc.end = current;
  *position = current;
  return node;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}
cubec_ast_node_t cubec_read_ast_expression_binary_prefix(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end) {
  cubec_ast_node_t err = NULL;
  cubec_ast_node_t node = NULL;
  cubec_position_t current = *position;
  node = cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_EXPRESSION_BINARY);
  cubec_ast_node_t opt = NULL;
  if (*current.offset == '&') {
    opt = cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_LITERAL_SYMBOL);
    opt->loc.begin = current;
    current.offset++;
    current.column++;
    opt->loc.end = current;
  } else {
    opt = cubec_read_ast_literal_symbol(allocator, &current, end);
    if (!opt) {
      goto onerror;
    }
    if (opt->type == CUBEC_NODE_TYPE_ERROR) {
      err = opt;
      goto onerror;
    }
  }
  cubec_ast_add_child(allocator, node, "opt", opt);
  if (!cubec_location_is(opt->loc, "!") && !cubec_location_is(opt->loc, "++") &&
      !cubec_location_is(opt->loc, "--") && !cubec_location_is(opt->loc, "+") &&
      !cubec_location_is(opt->loc, "-") && !cubec_location_is(opt->loc, "~") &&
      !cubec_location_is(opt->loc, "&") && !cubec_location_is(opt->loc, "*")) {
    goto onerror;
  }

  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_ast_node_t right =
      cubec_read_ast_expression15(allocator, &current, end);
  if (!right) {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Unexpected expression");
    goto onerror;
  }
  if (right->type == CUBEC_NODE_TYPE_ERROR) {
    err = right;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "right", right);
  node->loc.begin = *position;
  node->loc.end = current;
  *position = current;
  return node;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}
cubec_ast_node_t cubec_read_ast_expression_binary_postfix(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end) {

  cubec_ast_node_t err = NULL;
  cubec_ast_node_t node = NULL;
  cubec_position_t current = *position;
  node = cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_EXPRESSION_BINARY);
  cubec_ast_node_t left = cubec_read_ast_expression17(allocator, &current, end);
  if (!left) {
    goto onerror;
  }
  if (left->type == CUBEC_NODE_TYPE_ERROR) {
    err = left;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "left", left);
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_allocator_free(allocator, err);
  cubec_ast_node_t opt =
      cubec_read_ast_literal_symbol(allocator, &current, end);
  if (!opt) {
    err = cubec_map_move(node->children, "left", NULL);
    *position = err->loc.end;
    goto onerror;
  }
  if (opt->type == CUBEC_NODE_TYPE_ERROR) {
    err = opt;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "opt", opt);
  if (!cubec_location_is(opt->loc, "++") &&
      !cubec_location_is(opt->loc, "--")) {
    err = cubec_map_move(node->children, "left", NULL);
    *position = err->loc.end;
    goto onerror;
  }
  node->loc.begin = *position;
  node->loc.end = current;
  *position = current;
  return node;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}