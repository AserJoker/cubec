#include "ast/expression_assigment.h"
#include "ast/expression.h"
#include "ast/expression_binary.h"
#include "ast/literal_symbol.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"
static void
cubec_ast_expression_assigment_dispose(cubec_ast_expression_assigment_t self,
                                       cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->identifier);
  cubec_allocator_free(allocator, self->opt);
  cubec_allocator_free(allocator, self->value);
  cubec_ast_node_dispose(allocator, &self->super);
}
cubec_ast_expression_assigment_t
cubec_create_ast_expression_assigment(cubec_allocator_t allocator) {
  cubec_ast_expression_assigment_t node = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_ast_expression_assigment_t),
      (cubec_dispose_fn_t)cubec_ast_expression_assigment_dispose);
  cubec_ast_node_initialize(allocator, &node->super);
  cubec_ast_set_field(node, allocator, identifier);
  cubec_ast_set_field(node, allocator, value);
  cubec_ast_set_field(node, allocator, opt);
  node->super.type = CUBEC_NODE_TYPE_EXPRESSION_ASSIGMENT;
  return node;
}

cubec_ast_node_t cubec_read_ast_expression_assigment(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end) {
  static const char *opts[] = {
      "=",  "+=", "-=", "*=",  "/=",  "%=",  ">>=", "<<=",
      "&=", "|=", "^=", "&&=", "||=", "??=", NULL,
  };
  cubec_ast_expression_assigment_t node = NULL;
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  node = cubec_create_ast_expression_assigment(allocator);
  if (*current.offset == '*') {
    node->identifier =
        cubec_read_ast_expression_binary_prefix(allocator, position, end);
    if (!node->identifier) {
      goto onerror;
    }
  } else {
    node->identifier = cubec_read_ast_expression18(allocator, &current, end);
  }
  if (!node->identifier) {
    goto onerror;
  }
  if (node->identifier->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  if (node->identifier->type != CUBEC_NODE_TYPE_LITERAL_IDENTIFIER &&
      node->identifier->type != CUBEC_NODE_TYPE_EXPRESSION_MEMBER &&
      node->identifier->type != CUBEC_NODE_TYPE_EXPRESSION_COMPUTE_MEMBER) {
    goto onerror;
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_allocator_free(allocator, err);

  node->opt = cubec_read_ast_literal_symbol(allocator, &current, end);
  if (!node->opt) {
    goto onerror;
  }
  if (node->opt->type == CUBEC_NODE_TYPE_ERROR) {
    err = node->opt;
    node->opt = NULL;
    goto onerror;
  }
  size_t idx = 0;
  while (opts[idx] != NULL) {
    if (cubec_location_is(node->opt->loc, opts[idx])) {
      break;
    }
    idx++;
  }
  if (opts[idx] == NULL) {
    goto onerror;
  }

  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }

  node->value = cubec_read_ast_expression2(allocator, &current, end);
  if (!node->value) {
    err = cubec_create_ast_error(
        allocator, *position, current,
        "Invalid or unexpected token, missing initialize expression");
    goto onerror;
  }
  if (node->value->type == CUBEC_NODE_TYPE_ERROR) {
    err = node->value;
    node->value = NULL;
    goto onerror;
  }
  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  cubec_ast_set_parent(node->identifier, &node->super);
  cubec_ast_set_parent(node->opt, &node->super);
  cubec_ast_set_parent(node->value, &node->super);
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}