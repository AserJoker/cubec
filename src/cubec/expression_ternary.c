#include "cubec/expression_ternary.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/token.h"
#include "cubec/expression.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include <inttypes.h>

/* Forward declaration for read_expression_binary */
extern node_t read_expression_binary(allocator_t allocator, vec_t tokens,
                                     size_t *position, const char *filename);

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_expression_ternary_init(cubec_expression_ternary_t self,
                                           allocator_t allocator,
                                           cubec_expression_ternary_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  cubec_expression_init_t super_init = {
      .kind = CUBEC_NODE_EXPRESSION_TERNARY,
      .parent = NULL,
  };
  super_init.location = init->location;
  super_init.parent = init->parent;
  TRY_VOID_LOCAL(onerror, g_cubec_expression_type.init(&self->super, allocator, &super_init));

  self->condition = init->condition;
  self->consequent = init->consequent;
  self->alternate = init->alternate;
onerror:
  return;
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
  TRY_VOID_LOCAL(cleanup, g_cubec_expression_type.clone(&self->super, allocator, &another->super));
  self->condition = TRY_LOCAL(cleanup, value_clone(allocator, another->condition));
  self->consequent = TRY_LOCAL(cleanup, value_clone(allocator, another->consequent));
  self->alternate = TRY_LOCAL(cleanup, value_clone(allocator, another->alternate));
  return;

cleanup:
  allocator_free(allocator, &self->alternate);
  allocator_free(allocator, &self->consequent);
  allocator_free(allocator, &self->condition);
}

static void _cubec_expression_ternary_move(cubec_expression_ternary_t self,
                                           allocator_t allocator,
                                           cubec_expression_ternary_t another) {
  TRY_VOID_LOCAL(cleanup, g_cubec_expression_type.move(&self->super, allocator, &another->super));
  self->condition = TRY_LOCAL(cleanup, value_move(allocator, another->condition));
  self->consequent = TRY_LOCAL(cleanup, value_move(allocator, another->consequent));
  self->alternate = TRY_LOCAL(cleanup, value_move(allocator, another->alternate));
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

node_t read_expression_ternary(allocator_t allocator, vec_t tokens,
                               size_t *position, const char *filename) {
  size_t current = *position;
  node_t condition = NULL;
  cubec_expression_ternary_t node = NULL;
  node_t consequent = NULL;
  node_t alternate = NULL;

  /* Parse condition using read_expression_binary.
   * Caller ensures whitespace is already skipped. */
  condition = TRY_LOCAL(onerror,read_expression_binary(allocator, tokens, &current, filename));
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

  /* Parse consequent expression (the true branch) */
  skip_whitespace(tokens, &current);
  consequent = TRY_LOCAL(onerror,
                         read_expression(allocator, tokens, &current, filename));
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
  alternate = TRY_LOCAL(onerror,
                        read_expression(allocator, tokens, &current, filename));
  if (!alternate) {
    goto onerror;
  }

  node = TRY_LOCAL(onerror, allocator_create(allocator, &g_cubec_expression_ternary_type,
                          &(cubec_expression_ternary_init_t){
                              .condition = condition,
                              .consequent = consequent,
                              .alternate = alternate,
                          }));

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
  /* condition may be NULL if error propagated from read_expression_binary —
   * in that case g_error is already set. Save location before freeing. */
  if (condition) {
    location_t loc = condition->location;
    allocator_free(allocator, &condition);
    allocator_free(allocator, &alternate);
    allocator_free(allocator, &consequent);
    allocator_free(allocator, &node);
    THROW(NULL, "%s:%" PRIuPTR ":%" PRIuPTR " invalid ternary expression",
          filename, loc.begin.line + 1, loc.begin.column + 1);
  }
  allocator_free(allocator, &condition);
  allocator_free(allocator, &alternate);
  allocator_free(allocator, &consequent);
  allocator_free(allocator, &node);
  return NULL;
}
