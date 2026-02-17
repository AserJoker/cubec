#include "ast/expression_binary.h"
#include "ast/expression.h"
#include "ast/literal_identifier.h"
#include "ast/literal_symbol.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"
static void
cubec_ast_expression_binary_dispose(cubec_ast_expression_binary_t self,
                                    cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->left);
  cubec_allocator_free(allocator, self->right);
  cubec_allocator_free(allocator, self->opt);
}

cubec_ast_expression_binary_t
cubec_create_ast_expression_binary(cubec_allocator_t allocator) {
  cubec_ast_expression_binary_t node = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_ast_expression_binary_t),
      (cubec_dispose_fn_t)cubec_ast_expression_binary_dispose);
  cubec_ast_node_initialize(allocator, &node->super);
  node->left = NULL;
  node->right = NULL;
  node->opt = NULL;
  node->super.type = CUBEC_NODE_TYPE_EXPRESSION_BINARY;
  return node;
}
cubec_ast_node_t cubec_read_ast_expression_binary_logical_or(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end) {
  cubec_ast_node_t err = NULL;
  cubec_ast_expression_binary_t node = NULL;
  cubec_position_t current = *position;
  node = cubec_create_ast_expression_binary(allocator);
  node->left = cubec_read_ast_expression5(allocator, &current, end);
  if (!node->left) {
    goto onerror;
  }
  if (node->left->type == CUBEC_NODE_TYPE_ERROR) {
    err = node->left;
    node->left = NULL;
    goto onerror;
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_allocator_free(allocator, err);
  node->opt = cubec_read_ast_literal_symbol(allocator, &current, end);
  if (!node->opt) {
    err = node->left;
    node->left = NULL;
    *position = err->loc.end;
    goto onerror;
  }
  if (node->opt->type == CUBEC_NODE_TYPE_ERROR) {
    err = node->opt;
    node->opt = NULL;
    goto onerror;
  }
  if (!cubec_location_is(node->opt->loc, "||") &&
      !cubec_location_is(node->opt->loc, "??")) {
    err = node->left;
    node->left = NULL;
    *position = err->loc.end;
    goto onerror;
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_allocator_free(allocator, err);
  node->right = cubec_read_ast_expression4(allocator, &current, end);
  if (!node->right) {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Unexpected expression");
    goto onerror;
  }
  if (node->right->type == CUBEC_NODE_TYPE_ERROR) {
    err = node->right;
    node->right = NULL;
    goto onerror;
  }
  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}

cubec_ast_node_t cubec_read_ast_expression_binary_logical_and(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end) {
  cubec_ast_node_t err = NULL;
  cubec_ast_expression_binary_t node = NULL;
  cubec_position_t current = *position;
  node = cubec_create_ast_expression_binary(allocator);
  node->left = cubec_read_ast_expression6(allocator, &current, end);
  if (!node->left) {
    goto onerror;
  }
  if (node->left->type == CUBEC_NODE_TYPE_ERROR) {
    err = node->left;
    node->left = NULL;
    goto onerror;
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_allocator_free(allocator, err);
  node->opt = cubec_read_ast_literal_symbol(allocator, &current, end);
  if (!node->opt) {
    err = node->left;
    node->left = NULL;
    *position = err->loc.end;
    goto onerror;
  }
  if (node->opt->type == CUBEC_NODE_TYPE_ERROR) {
    err = node->opt;
    node->opt = NULL;
    goto onerror;
  }
  if (!cubec_location_is(node->opt->loc, "&&")) {
    err = node->left;
    node->left = NULL;
    *position = err->loc.end;
    goto onerror;
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_allocator_free(allocator, err);
  node->right = cubec_read_ast_expression5(allocator, &current, end);
  if (!node->right) {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Unexpected expression");
    goto onerror;
  }
  if (node->right->type == CUBEC_NODE_TYPE_ERROR) {
    err = node->right;
    node->right = NULL;
    goto onerror;
  }
  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}

cubec_ast_node_t cubec_read_ast_expression_binary_bitwise_or(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end) {
  cubec_ast_node_t err = NULL;
  cubec_ast_expression_binary_t node = NULL;
  cubec_position_t current = *position;
  node = cubec_create_ast_expression_binary(allocator);
  node->left = cubec_read_ast_expression7(allocator, &current, end);
  if (!node->left) {
    goto onerror;
  }
  if (node->left->type == CUBEC_NODE_TYPE_ERROR) {
    err = node->left;
    node->left = NULL;
    goto onerror;
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_allocator_free(allocator, err);
  node->opt = cubec_read_ast_literal_symbol(allocator, &current, end);
  if (!node->opt) {
    err = node->left;
    node->left = NULL;
    *position = err->loc.end;
    goto onerror;
  }
  if (node->opt->type == CUBEC_NODE_TYPE_ERROR) {
    err = node->opt;
    node->opt = NULL;
    goto onerror;
  }
  if (!cubec_location_is(node->opt->loc, "|")) {
    err = node->left;
    node->left = NULL;
    *position = err->loc.end;
    goto onerror;
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_allocator_free(allocator, err);
  node->right = cubec_read_ast_expression7(allocator, &current, end);
  if (!node->right) {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Unexpected expression");
    goto onerror;
  }
  if (node->right->type == CUBEC_NODE_TYPE_ERROR) {
    err = node->right;
    node->right = NULL;
    goto onerror;
  }
  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}
cubec_ast_node_t cubec_read_ast_expression_binary_bitwise_xor(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end) {
  cubec_ast_node_t err = NULL;
  cubec_ast_expression_binary_t node = NULL;
  cubec_position_t current = *position;
  node = cubec_create_ast_expression_binary(allocator);
  node->left = cubec_read_ast_expression8(allocator, &current, end);
  if (!node->left) {
    goto onerror;
  }
  if (node->left->type == CUBEC_NODE_TYPE_ERROR) {
    err = node->left;
    node->left = NULL;
    goto onerror;
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_allocator_free(allocator, err);
  node->opt = cubec_read_ast_literal_symbol(allocator, &current, end);
  if (!node->opt) {
    err = node->left;
    node->left = NULL;
    *position = err->loc.end;
    goto onerror;
  }
  if (node->opt->type == CUBEC_NODE_TYPE_ERROR) {
    err = node->opt;
    node->opt = NULL;
    goto onerror;
  }
  if (!cubec_location_is(node->opt->loc, "^")) {
    err = node->left;
    node->left = NULL;
    *position = err->loc.end;
    goto onerror;
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_allocator_free(allocator, err);
  node->right = cubec_read_ast_expression8(allocator, &current, end);
  if (!node->right) {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Unexpected expression");
    goto onerror;
  }
  if (node->right->type == CUBEC_NODE_TYPE_ERROR) {
    err = node->right;
    node->right = NULL;
    goto onerror;
  }
  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}
cubec_ast_node_t cubec_read_ast_expression_binary_bitwise_and(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end) {
  cubec_ast_node_t err = NULL;
  cubec_ast_expression_binary_t node = NULL;
  cubec_position_t current = *position;
  node = cubec_create_ast_expression_binary(allocator);
  node->left = cubec_read_ast_expression9(allocator, &current, end);
  if (!node->left) {
    goto onerror;
  }
  if (node->left->type == CUBEC_NODE_TYPE_ERROR) {
    err = node->left;
    node->left = NULL;
    goto onerror;
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_allocator_free(allocator, err);
  node->opt = cubec_read_ast_literal_symbol(allocator, &current, end);
  if (!node->opt) {
    err = node->left;
    node->left = NULL;
    *position = err->loc.end;
    goto onerror;
  }
  if (node->opt->type == CUBEC_NODE_TYPE_ERROR) {
    err = node->opt;
    node->opt = NULL;
    goto onerror;
  }
  if (!cubec_location_is(node->opt->loc, "&")) {
    err = node->left;
    node->left = NULL;
    *position = err->loc.end;
    goto onerror;
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_allocator_free(allocator, err);
  node->right = cubec_read_ast_expression9(allocator, &current, end);
  if (!node->right) {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Unexpected expression");
    goto onerror;
  }
  if (node->right->type == CUBEC_NODE_TYPE_ERROR) {
    err = node->right;
    node->right = NULL;
    goto onerror;
  }
  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}
cubec_ast_node_t cubec_read_ast_expression_binary_equal(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end) {

  cubec_ast_node_t err = NULL;
  cubec_ast_expression_binary_t node = NULL;
  cubec_position_t current = *position;
  node = cubec_create_ast_expression_binary(allocator);
  node->left = cubec_read_ast_expression10(allocator, &current, end);
  if (!node->left) {
    goto onerror;
  }
  if (node->left->type == CUBEC_NODE_TYPE_ERROR) {
    err = node->left;
    node->left = NULL;
    goto onerror;
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_allocator_free(allocator, err);
  node->opt = cubec_read_ast_literal_symbol(allocator, &current, end);
  if (!node->opt) {
    err = node->left;
    node->left = NULL;
    *position = err->loc.end;
    goto onerror;
  }
  if (node->opt->type == CUBEC_NODE_TYPE_ERROR) {
    err = node->opt;
    node->opt = NULL;
    goto onerror;
  }
  if (!cubec_location_is(node->opt->loc, "==") &&
      !cubec_location_is(node->opt->loc, "!=")) {
    err = node->left;
    node->left = NULL;
    *position = err->loc.end;
    goto onerror;
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_allocator_free(allocator, err);
  node->right = cubec_read_ast_expression10(allocator, &current, end);
  if (!node->right) {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Unexpected expression");
    goto onerror;
  }
  if (node->right->type == CUBEC_NODE_TYPE_ERROR) {
    err = node->right;
    node->right = NULL;
    goto onerror;
  }
  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}
cubec_ast_node_t cubec_read_ast_expression_binary_relation(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end) {

  cubec_ast_node_t err = NULL;
  cubec_ast_expression_binary_t node = NULL;
  cubec_position_t current = *position;
  node = cubec_create_ast_expression_binary(allocator);
  node->left = cubec_read_ast_expression11(allocator, &current, end);
  if (!node->left) {
    goto onerror;
  }
  if (node->left->type == CUBEC_NODE_TYPE_ERROR) {
    err = node->left;
    node->left = NULL;
    goto onerror;
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_allocator_free(allocator, err);
  node->opt = cubec_read_ast_literal_symbol(allocator, &current, end);
  if (!node->opt) {
    err = node->left;
    node->left = NULL;
    *position = err->loc.end;
    goto onerror;
  }
  if (node->opt->type == CUBEC_NODE_TYPE_ERROR) {
    err = node->opt;
    node->opt = NULL;
    goto onerror;
  }
  if (!cubec_location_is(node->opt->loc, ">") &&
      !cubec_location_is(node->opt->loc, "<") &&
      !cubec_location_is(node->opt->loc, ">=") &&
      !cubec_location_is(node->opt->loc, "<=")) {
    err = node->left;
    node->left = NULL;
    *position = err->loc.end;
    goto onerror;
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_allocator_free(allocator, err);
  node->right = cubec_read_ast_expression11(allocator, &current, end);
  if (!node->right) {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Unexpected expression");
    goto onerror;
  }
  if (node->right->type == CUBEC_NODE_TYPE_ERROR) {
    err = node->right;
    node->right = NULL;
    goto onerror;
  }
  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}
cubec_ast_node_t cubec_read_ast_expression_binary_bitwise_shift(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end) {

  cubec_ast_node_t err = NULL;
  cubec_ast_expression_binary_t node = NULL;
  cubec_position_t current = *position;
  node = cubec_create_ast_expression_binary(allocator);
  node->left = cubec_read_ast_expression12(allocator, &current, end);
  if (!node->left) {
    goto onerror;
  }
  if (node->left->type == CUBEC_NODE_TYPE_ERROR) {
    err = node->left;
    node->left = NULL;
    goto onerror;
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_allocator_free(allocator, err);
  node->opt = cubec_read_ast_literal_symbol(allocator, &current, end);
  if (!node->opt) {
    err = node->left;
    node->left = NULL;
    *position = err->loc.end;
    goto onerror;
  }
  if (node->opt->type == CUBEC_NODE_TYPE_ERROR) {
    err = node->opt;
    node->opt = NULL;
    goto onerror;
  }
  if (!cubec_location_is(node->opt->loc, ">>") &&
      !cubec_location_is(node->opt->loc, "<<")) {
    err = node->left;
    node->left = NULL;
    *position = err->loc.end;
    goto onerror;
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_allocator_free(allocator, err);
  node->right = cubec_read_ast_expression12(allocator, &current, end);
  if (!node->right) {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Unexpected expression");
    goto onerror;
  }
  if (node->right->type == CUBEC_NODE_TYPE_ERROR) {
    err = node->right;
    node->right = NULL;
    goto onerror;
  }
  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}
cubec_ast_node_t cubec_read_ast_expression_binary_additive(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end) {
  cubec_ast_node_t err = NULL;
  cubec_ast_expression_binary_t node = NULL;
  cubec_position_t current = *position;
  node = cubec_create_ast_expression_binary(allocator);
  node->left = cubec_read_ast_expression13(allocator, &current, end);
  if (!node->left) {
    goto onerror;
  }
  if (node->left->type == CUBEC_NODE_TYPE_ERROR) {
    err = node->left;
    node->left = NULL;
    goto onerror;
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_allocator_free(allocator, err);
  node->opt = cubec_read_ast_literal_symbol(allocator, &current, end);
  if (!node->opt) {
    err = node->left;
    node->left = NULL;
    *position = err->loc.end;
    goto onerror;
  }
  if (node->opt->type == CUBEC_NODE_TYPE_ERROR) {
    err = node->opt;
    node->opt = NULL;
    goto onerror;
  }
  if (!cubec_location_is(node->opt->loc, "+") &&
      !cubec_location_is(node->opt->loc, "-")) {
    err = node->left;
    node->left = NULL;
    *position = err->loc.end;
    goto onerror;
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_allocator_free(allocator, err);
  node->right = cubec_read_ast_expression13(allocator, &current, end);
  if (!node->right) {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Unexpected expression");
    goto onerror;
  }
  if (node->right->type == CUBEC_NODE_TYPE_ERROR) {
    err = node->right;
    node->right = NULL;
    goto onerror;
  }
  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}
cubec_ast_node_t cubec_read_ast_expression_binary_multiplicative(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end) {
  cubec_ast_node_t err = NULL;
  cubec_ast_expression_binary_t node = NULL;
  cubec_position_t current = *position;
  node = cubec_create_ast_expression_binary(allocator);
  node->left = cubec_read_ast_expression14(allocator, &current, end);
  if (!node->left) {
    goto onerror;
  }
  if (node->left->type == CUBEC_NODE_TYPE_ERROR) {
    err = node->left;
    node->left = NULL;
    goto onerror;
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_allocator_free(allocator, err);
  node->opt = cubec_read_ast_literal_symbol(allocator, &current, end);
  if (!node->opt) {
    err = node->left;
    node->left = NULL;
    *position = err->loc.end;
    goto onerror;
  }
  if (node->opt->type == CUBEC_NODE_TYPE_ERROR) {
    err = node->opt;
    node->opt = NULL;
    goto onerror;
  }
  if (!cubec_location_is(node->opt->loc, "*") &&
      !cubec_location_is(node->opt->loc, "/") &&
      !cubec_location_is(node->opt->loc, "%")) {
    err = node->left;
    node->left = NULL;
    *position = err->loc.end;
    goto onerror;
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_allocator_free(allocator, err);
  node->right = cubec_read_ast_expression14(allocator, &current, end);
  if (!node->right) {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Unexpected expression");
    goto onerror;
  }
  if (node->right->type == CUBEC_NODE_TYPE_ERROR) {
    err = node->right;
    node->right = NULL;
    goto onerror;
  }
  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}
cubec_ast_node_t cubec_read_ast_expression_binary_prefix(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end) {
  cubec_ast_node_t err = NULL;
  cubec_ast_expression_binary_t node = NULL;
  cubec_position_t current = *position;
  node = cubec_create_ast_expression_binary(allocator);
  node->opt = cubec_read_ast_literal_symbol(allocator, &current, end);
  if (!node->opt) {
    node->opt = cubec_read_ast_literal_identifier(allocator, &current, end);
  }
  if (!node->opt) {
    goto onerror;
  }
  if (node->opt->type == CUBEC_NODE_TYPE_ERROR) {
    err = node->opt;
    node->opt = NULL;
    goto onerror;
  }
  if (!cubec_location_is(node->opt->loc, "!") &&
      !cubec_location_is(node->opt->loc, "++") &&
      !cubec_location_is(node->opt->loc, "--") &&
      !cubec_location_is(node->opt->loc, "+") &&
      !cubec_location_is(node->opt->loc, "-") &&
      !cubec_location_is(node->opt->loc, "~") &&
      !cubec_location_is(node->opt->loc, "&") &&
      !cubec_location_is(node->opt->loc, "try") &&
      !cubec_location_is(node->opt->loc, "typeof") &&
      !cubec_location_is(node->opt->loc, "sizeof")) {
    goto onerror;
  }

  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_allocator_free(allocator, err);
  node->right = cubec_read_ast_expression16(allocator, &current, end);
  if (!node->right) {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Unexpected expression");
    goto onerror;
  }
  if (node->right->type == CUBEC_NODE_TYPE_ERROR) {
    err = node->right;
    node->right = NULL;
    goto onerror;
  }
  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}
cubec_ast_node_t cubec_read_ast_expression_binary_postfix(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end) {

  cubec_ast_node_t err = NULL;
  cubec_ast_expression_binary_t node = NULL;
  cubec_position_t current = *position;
  node = cubec_create_ast_expression_binary(allocator);
  node->left = cubec_read_ast_expression17(allocator, &current, end);
  if (!node->left) {
    goto onerror;
  }
  if (node->left->type == CUBEC_NODE_TYPE_ERROR) {
    err = node->left;
    node->left = NULL;
    goto onerror;
  }

  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_allocator_free(allocator, err);
  node->opt = cubec_read_ast_literal_symbol(allocator, &current, end);
  if (!node->opt) {
    err = node->left;
    node->left = NULL;
    *position = err->loc.end;
    goto onerror;
  }
  if (node->opt->type == CUBEC_NODE_TYPE_ERROR) {
    err = node->opt;
    node->opt = NULL;
    goto onerror;
  }
  if (!cubec_location_is(node->opt->loc, "++") &&
      !cubec_location_is(node->opt->loc, "--")) {
    err = node->left;
    node->left = NULL;
    *position = err->loc.end;
    goto onerror;
  }
  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}