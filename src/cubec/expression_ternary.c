#include "cubec/expression_ternary.h"
#include "core/allocator.h"
#include "core/token.h"
#include "cubec/ast_factory.h"
#include "cubec/ast_factory_internal.h"
#include "cubec/expression.h"
#include "cubec/node.h"
#include "cubec/node_error.h"
#include "cubec/token.h"
#include <inttypes.h>
#include "engine/context.h"
#include "engine/diagnostic.h"

/* Forward declaration for read_expression_binary */
extern node_t read_expression_binary(context_t ctx, vec_t tokens,
                                     size_t *position, const char *filename);

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_expression_ternary_init(cubec_expression_ternary_t self,
                                           allocator_t allocator,
                                           cubec_expression_ternary_init_t *init) {
  cubec_expression_init_t super_init = {
      .kind = CUBEC_NODE_EXPRESSION_TERNARY,
      .parent = NULL,
  };
  super_init.location = init->location;
  super_init.parent = init->parent;
  g_cubec_expression_type.init(&self->super, allocator, &super_init);

  self->condition = init->condition;
  self->consequent = init->consequent;
  self->alternate = init->alternate;
}

static void _cubec_expression_ternary_dispose(cubec_expression_ternary_t self,
                                              allocator_t allocator) {
  allocator_free(allocator, &self->condition);
  allocator_free(allocator, &self->consequent);
  allocator_free(allocator, &self->alternate);
  g_cubec_expression_type.dispose(&self->super, allocator);
}

static void _cubec_expression_ternary_clone(cubec_expression_ternary_t self,
                                            allocator_t allocator,
                                            cubec_expression_ternary_t another) {
  g_cubec_expression_type.clone(&self->super, allocator, &another->super);
  self->condition = value_clone(allocator, another->condition);
  self->consequent = value_clone(allocator, another->consequent);
  self->alternate = value_clone(allocator, another->alternate);
  return;

cleanup:
  allocator_free(allocator, &self->alternate);
  allocator_free(allocator, &self->consequent);
  allocator_free(allocator, &self->condition);
}

static void _cubec_expression_ternary_move(cubec_expression_ternary_t self,
                                           allocator_t allocator,
                                           cubec_expression_ternary_t another) {
  g_cubec_expression_type.move(&self->super, allocator, &another->super);
  self->condition = value_move(allocator, another->condition);
  self->consequent = value_move(allocator, another->consequent);
  self->alternate = value_move(allocator, another->alternate);
  return;

cleanup:
  allocator_free(allocator, &self->alternate);
  allocator_free(allocator, &self->consequent);
  allocator_free(allocator, &self->condition);
}

type_t g_cubec_expression_ternary_type = {
    .name = "cubec.cubec.expression_ternary",
    .size = sizeof(struct _cubec_expression_ternary_t),
    .init = (type_init_fn_t)_cubec_expression_ternary_init,
    .dispose = (type_dispose_fn_t)_cubec_expression_ternary_dispose,
    .clone = (type_clone_fn_t)_cubec_expression_ternary_clone,
    .move = (type_move_fn_t)_cubec_expression_ternary_move,
};

/* --------------------------------------------------------------------------
 *  Parser: read_expression_ternary
 * -------------------------------------------------------------------------- */

node_t read_expression_ternary(context_t ctx, vec_t tokens,
                               size_t *position, const char *filename) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;
  node_t condition = NULL;
  cubec_expression_ternary_t node = NULL;
  node_t consequent = NULL;
  node_t alternate = NULL;

  /* Parse condition using read_expression_binary.
   * This handles all binary ops including ==, !=, extends.
   * If no '?' follows, returns the condition as-is. */
  condition = read_expression_binary(ctx, tokens, &current, filename);
  if (node_is_error(condition)) return condition;
  if (!condition) {
    return NULL;
  }

  /* Check if this is actually a ternary — expect '?' */
  skip_whitespace(tokens, &current);
  if (!token_is(vec_get(tokens, current), CUBEC_TOKEN_SYMBOL, "?")) {
    /* Not a ternary — return the condition as-is */
    *position = current;
    return condition;
  }
  current++; /* Consumed '?' — committed to parsing a ternary from here */
  location_t start_location = condition->location;
  start_location.filename = filename;

  /* Parse consequent expression (the true branch) */
  skip_whitespace(tokens, &current);
  consequent = read_expression(ctx, tokens, &current, filename);
  if (node_is_error(consequent)) {
    allocator_free(allocator, &condition);
    return consequent;
  }
  if (!consequent) {
    goto onerror;
  }

  /* Expect ':' */
  skip_whitespace(tokens, &current);
  token_t colon = vec_get(tokens, current);
  if (!token_is(colon, CUBEC_TOKEN_SYMBOL, ":")) {
    goto onerror;
  }
  current++;

  /* Parse alternate expression (the false branch) via read_expression
   * to handle nested ternaries naturally */
  skip_whitespace(tokens, &current);
  alternate = read_expression(ctx, tokens, &current, filename);
  if (node_is_error(alternate)) {
    allocator_free(allocator, &condition);
    allocator_free(allocator, &consequent);
    return alternate;
  }
  if (!alternate) {
    goto onerror;
  }

  node = allocator_create(allocator, &g_cubec_expression_ternary_type,
                          &(cubec_expression_ternary_init_t){
                              .condition = condition,
                              .consequent = consequent,
                              .alternate = alternate,
                          });

  /* Location spans from condition start to alternate end */
  {
    location_t loc = condition->location;
    loc.end = token_get_location(colon)->end;
    node->super.super.location = loc;
    node->super.super.location.filename = filename;
  }

  *position = current;
  return (node_t)node;

onerror:
  allocator_free(allocator, &condition);
  allocator_free(allocator, &alternate);
  allocator_free(allocator, &consequent);
  allocator_free(allocator, &node);
  return cubec_ast_create_error(ctx, start_location);
}

/* --------------------------------------------------------------------------
 *  Factory: cubec_ast_create_ternary
 * -------------------------------------------------------------------------- */

node_t cubec_ast_create_ternary(context_t ctx, location_t loc,
                                node_t cond, node_t then_branch,
                                node_t else_branch) {
  allocator_t alloc = ctx->allocator;
      cubec_expression_ternary_init_t init = {
      .location = loc, .parent = NULL, .condition = cond,
      .consequent = then_branch, .alternate = else_branch};
  return (node_t)allocator_create(alloc, &g_cubec_expression_ternary_type,
                                  &init);
}
