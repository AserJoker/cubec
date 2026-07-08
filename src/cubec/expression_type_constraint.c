#include "cubec/expression_type_constraint.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/token.h"
#include "cubec/expression.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_expression_type_constraint_init(
    cubec_expression_type_constraint_t self, allocator_t allocator,
    cubec_expression_type_constraint_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  cubec_expression_init_t super_init = {
      .kind = CUBEC_NODE_EXPRESSION_TYPE_CONSTRAINT,
      .parent = NULL,
  };
  super_init.location = init->location;
  super_init.parent = init->parent;
  TRY_VOID_LOCAL(onerror,
                 g_cubec_expression_type.init(&self->super, allocator,
                                              &super_init));
  self->op = init->op;
  self->left = init->left;
  self->right = init->right;
onerror:
  return;
}

static void _cubec_expression_type_constraint_dispose(
    cubec_expression_type_constraint_t self, allocator_t allocator) {
  allocator_free(allocator, &self->left);
  allocator_free(allocator, &self->right);
  g_cubec_expression_type.dispose(&self->super, allocator);
}

static void _cubec_expression_type_constraint_clone(
    cubec_expression_type_constraint_t self, allocator_t allocator,
    cubec_expression_type_constraint_t another) {
  TRY_VOID_LOCAL(
      cleanup,
      g_cubec_expression_type.clone(&self->super, allocator, &another->super));
  self->op = another->op;
  self->left =
      TRY_LOCAL(cleanup, value_clone(allocator, another->left));
  self->right =
      TRY_LOCAL(cleanup, value_clone(allocator, another->right));
  return;

cleanup:
  allocator_free(allocator, &self->right);
  allocator_free(allocator, &self->left);
}

static void _cubec_expression_type_constraint_move(
    cubec_expression_type_constraint_t self, allocator_t allocator,
    cubec_expression_type_constraint_t another) {
  TRY_VOID_LOCAL(
      cleanup,
      g_cubec_expression_type.move(&self->super, allocator, &another->super));
  self->op = another->op;
  self->left =
      TRY_LOCAL(cleanup, value_move(allocator, another->left));
  self->right =
      TRY_LOCAL(cleanup, value_move(allocator, another->right));
  return;

cleanup:
  allocator_free(allocator, &self->right);
  allocator_free(allocator, &self->left);
}

type_t g_cubec_expression_type_constraint_type = {
    .name = "cubec.cubec.expression_type_constraint",
    .size = sizeof(struct _cubec_expression_type_constraint_t),
    .init = (type_init_fn_t)_cubec_expression_type_constraint_init,
    .dispose = (type_dispose_fn_t)_cubec_expression_type_constraint_dispose,
    .clone = (type_clone_fn_t)_cubec_expression_type_constraint_clone,
    .move = (type_move_fn_t)_cubec_expression_type_constraint_move,
};

/* --------------------------------------------------------------------------
 *  Parser: read_expression_type_constraint
 * -------------------------------------------------------------------------- */

node_t read_expression_type_constraint(allocator_t allocator, vec_t tokens,
                                       size_t *position, const char *filename) {
  size_t current = *position;
  node_t left = NULL;
  cubec_expression_type_constraint_t node = NULL;
  node_t right = NULL;

  /* 1. Parse left operand */
  left = read_type_expression_primary(allocator, tokens, &current, filename);
  if (!left) {
    return NULL;
  }

  /* 2. Check for constraint operator */
  skip_whitespace(tokens, &current);
  token_t op_token = vec_get(tokens, current);
  cubec_type_constraint_operator_t op;

  if (token_is(op_token, CUBEC_TOKEN_KEYWORD, "extends")) {
    op = CUBEC_TYPE_CONSTRAINT_EXTENDS;
  } else if (token_is(op_token, CUBEC_TOKEN_SYMBOL, "==")) {
    op = CUBEC_TYPE_CONSTRAINT_EQ;
  } else if (token_is(op_token, CUBEC_TOKEN_SYMBOL, "!=")) {
    op = CUBEC_TYPE_CONSTRAINT_NE;
  } else {
    /* No constraint operator — return left operand as-is */
    *position = current;
    return left;
  }
  current++;

  /* 3. Parse right operand (use primary to avoid consuming '?' for ternary) */
  skip_whitespace(tokens, &current);
  right = TRY_LOCAL(onerror,
                    read_type_expression_primary(allocator, tokens, &current,
                                                 filename));
  if (!right) {
    goto onerror;
  }

  /* 4. Build constraint node */
  node = TRY_LOCAL(
      onerror,
      allocator_create(allocator, &g_cubec_expression_type_constraint_type,
                       &(cubec_expression_type_constraint_init_t){
                           .op = op,
                           .left = left,
                           .right = right,
                       }));

  /* Location spans from left start to right end */
  {
    location_t loc = left->location;
    location_t *right_loc = token_get_location(
        vec_get(tokens, current - 1));
    loc.end = right_loc->end;
    node->super.super.location = loc;
    node->super.super.location.filename = filename;
  }

  *position = current;
  return (node_t)node;

onerror:
  allocator_free(allocator, &left);
  allocator_free(allocator, &right);
  allocator_free(allocator, &node);
  return NULL;
}
