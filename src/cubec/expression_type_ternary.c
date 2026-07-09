#include "cubec/expression_type_ternary.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/token.h"
#include "cubec/expression.h"
#include "cubec/expression_group.h"
#include "cubec/expression_type_constraint.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_expression_type_ternary_init(
    cubec_expression_type_ternary_t self, allocator_t allocator,
    cubec_expression_type_ternary_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  cubec_expression_init_t super_init = {
      .kind = CUBEC_NODE_EXPRESSION_TYPE_TERNARY,
      .parent = NULL,
  };
  super_init.location = init->location;
  super_init.parent = init->parent;
  TRY_VOID_LOCAL(onerror,
                 g_cubec_expression_type.init(&self->super, allocator, &super_init));

  self->condition = init->condition;
  self->consequent = init->consequent;
  self->alternate = init->alternate;
onerror:
  return;
}

static void _cubec_expression_type_ternary_dispose(
    cubec_expression_type_ternary_t self, allocator_t allocator) {
  allocator_free(allocator, &self->condition);
  allocator_free(allocator, &self->consequent);
  allocator_free(allocator, &self->alternate);
  g_cubec_expression_type.dispose(&self->super, allocator);
}

static void _cubec_expression_type_ternary_clone(
    cubec_expression_type_ternary_t self, allocator_t allocator,
    cubec_expression_type_ternary_t another) {
  TRY_VOID_LOCAL(cleanup, g_cubec_expression_type.clone(&self->super, allocator,
                                                        &another->super));
  self->condition =
      TRY_LOCAL(cleanup, value_clone(allocator, another->condition));
  self->consequent =
      TRY_LOCAL(cleanup, value_clone(allocator, another->consequent));
  self->alternate =
      TRY_LOCAL(cleanup, value_clone(allocator, another->alternate));
  return;

cleanup:
  allocator_free(allocator, &self->alternate);
  allocator_free(allocator, &self->consequent);
  allocator_free(allocator, &self->condition);
}

static void _cubec_expression_type_ternary_move(
    cubec_expression_type_ternary_t self, allocator_t allocator,
    cubec_expression_type_ternary_t another) {
  TRY_VOID_LOCAL(cleanup, g_cubec_expression_type.move(&self->super, allocator,
                                                       &another->super));
  self->condition =
      TRY_LOCAL(cleanup, value_move(allocator, another->condition));
  self->consequent =
      TRY_LOCAL(cleanup, value_move(allocator, another->consequent));
  self->alternate =
      TRY_LOCAL(cleanup, value_move(allocator, another->alternate));
  return;

cleanup:
  allocator_free(allocator, &self->alternate);
  allocator_free(allocator, &self->consequent);
  allocator_free(allocator, &self->condition);
}

type_t g_cubec_expression_type_ternary_type = {
    .name = "cubec.cubec.expression_type_ternary",
    .size = sizeof(struct _cubec_expression_type_ternary_t),
    .init = (type_init_fn_t)_cubec_expression_type_ternary_init,
    .dispose = (type_dispose_fn_t)_cubec_expression_type_ternary_dispose,
    .clone = (type_clone_fn_t)_cubec_expression_type_ternary_clone,
    .move = (type_move_fn_t)_cubec_expression_type_ternary_move,
};

/* --------------------------------------------------------------------------
 *  Parser: read_expression_type_ternary
 * -------------------------------------------------------------------------- */

node_t read_expression_type_ternary(allocator_t allocator, vec_t tokens,
                                    size_t *position, const char *filename) {
  size_t current = *position;
  node_t condition = NULL;
  cubec_expression_type_ternary_t node = NULL;
  node_t consequent = NULL;
  node_t alternate = NULL;

  /* Parse condition:
   * 1. Type constraint: T extends U, T == U, T != U — structured type
   *    constraint for compile-time type branching
   * 2. Expression group: ( expression ) — supports compile-time expressions
   *    (type_group is handled internally by read_type_expression_primary)
   * 3. Primary type expression: identifier, pointer, slice, array */
  condition =
      read_expression_type_constraint(allocator, tokens, &current, filename);
  if (!condition) {
    condition =
        read_expression_group(allocator, tokens, &current, filename);
  }
  if (!condition) {
    condition = read_type_expression_primary(allocator, tokens, &current,
                                             filename);
  }
  if (!condition) {
    return NULL;
  }

  /* Check if this is actually a ternary type — expect '?' */
  skip_whitespace(tokens, &current);
  if (!token_is(vec_get(tokens, current), CUBEC_TOKEN_SYMBOL, "?")) {
    /* Not a ternary type. Bare type constraints (A extends B without '?')
     * are only valid in generic definition context — error here. */
    if (condition->kind == CUBEC_NODE_EXPRESSION_TYPE_CONSTRAINT) {
      location_t loc = condition->location;
      allocator_free(allocator, &condition);
      THROW(NULL, "%s:%" PRIuPTR ":%" PRIuPTR
            " bare type constraint requires ternary form: "
            "'constraint ? consequent : alternate'",
            filename, loc.begin.line + 1, loc.begin.column + 1);
    }
    *position = current;
    return condition;
  }
  current++; /* Consumed '?' — committed to parsing a ternary from here */

  /* Parse consequent type expression (the true branch) */
  skip_whitespace(tokens, &current);
  consequent = TRY_LOCAL(onerror,
                         read_expression_type(allocator, tokens, &current,
                                              filename));
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

  /* Parse alternate type expression (the false branch) via read_expression_type
   * to handle nested ternaries naturally */
  skip_whitespace(tokens, &current);
  alternate = TRY_LOCAL(onerror,
                        read_expression_type(allocator, tokens, &current,
                                             filename));
  if (!alternate) {
    goto onerror;
  }

  node = TRY_LOCAL(
      onerror,
      allocator_create(allocator, &g_cubec_expression_type_ternary_type,
                       &(cubec_expression_type_ternary_init_t){
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
  if (condition) {
    location_t loc = condition->location;
    allocator_free(allocator, &condition);
    allocator_free(allocator, &alternate);
    allocator_free(allocator, &consequent);
    allocator_free(allocator, &node);
    THROW(NULL, "%s:%" PRIuPTR ":%" PRIuPTR " invalid ternary type expression",
          filename, loc.begin.line + 1, loc.begin.column + 1);
  }
  allocator_free(allocator, &condition);
  allocator_free(allocator, &alternate);
  allocator_free(allocator, &consequent);
  allocator_free(allocator, &node);
  return NULL;
}
