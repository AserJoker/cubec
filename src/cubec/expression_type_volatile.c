#include "cubec/expression_type_volatile.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/token.h"
#include "cubec/expression.h"
#include "cubec/node.h"
#include "cubec/token.h"

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_expression_type_volatile_init(
    cubec_expression_type_volatile_t self, allocator_t allocator,
    cubec_expression_type_volatile_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  cubec_expression_init_t super_init = {
      .kind = CUBEC_NODE_EXPRESSION_TYPE_VOLATILE,
      .parent = NULL,
  };
  super_init.location = init->location;
  super_init.parent = init->parent;
  TRY_VOID_LOCAL(onerror,
                 g_cubec_expression_type.init(&self->super, allocator, &super_init));
  self->type = init->type;
onerror:
  return;
}

static void _cubec_expression_type_volatile_dispose(
    cubec_expression_type_volatile_t self, allocator_t allocator) {
  allocator_free(allocator, &self->type);
  g_cubec_expression_type.dispose(&self->super, allocator);
}

static void _cubec_expression_type_volatile_clone(
    cubec_expression_type_volatile_t self, allocator_t allocator,
    cubec_expression_type_volatile_t another) {
  TRY_VOID_LOCAL(
      cleanup,
      g_cubec_expression_type.clone(&self->super, allocator, &another->super));
  self->type = TRY_LOCAL(cleanup, value_clone(allocator, another->type));
  return;

cleanup:
  allocator_free(allocator, &self->type);
}

static void _cubec_expression_type_volatile_move(
    cubec_expression_type_volatile_t self, allocator_t allocator,
    cubec_expression_type_volatile_t another) {
  TRY_VOID_LOCAL(
      cleanup,
      g_cubec_expression_type.move(&self->super, allocator, &another->super));
  self->type = TRY_LOCAL(cleanup, value_move(allocator, another->type));
  return;

cleanup:
  allocator_free(allocator, &self->type);
}

type_t g_cubec_expression_type_volatile_type = {
    .name = "cubec.cubec.expression_type_volatile",
    .size = sizeof(struct _cubec_expression_type_volatile_t),
    .init = (type_init_fn_t)_cubec_expression_type_volatile_init,
    .dispose = (type_dispose_fn_t)_cubec_expression_type_volatile_dispose,
    .clone = (type_clone_fn_t)_cubec_expression_type_volatile_clone,
    .move = (type_move_fn_t)_cubec_expression_type_volatile_move,
};

/* --------------------------------------------------------------------------
 *  Parser: read_expression_type_volatile
 * -------------------------------------------------------------------------- */

node_t read_expression_type_volatile(allocator_t allocator, vec_t tokens,
                                     size_t *position, const char *filename) {
  size_t current = *position;
  cubec_expression_type_volatile_t node = NULL;
  node_t type = NULL;

  /* Expect 'volatile' keyword */
  token_t volatile_token = vec_get(tokens, current);
  if (!volatile_token ||
      !token_is(volatile_token, CUBEC_TOKEN_KEYWORD, "volatile")) {
    return NULL;
  }
  current++;

  /* Skip whitespace after 'volatile' */
  skip_whitespace(tokens, &current);

  /* Parse the underlying type using read_type_expression_primary.
   * Ternary type expressions are not allowed directly as volatile base type;
   * use type_group to wrap them: volatile (a ? b : c). */
  type = TRY_LOCAL(onerror,
                   read_type_expression_primary(allocator, tokens, &current, filename));
  if (!type) {
    THROW_LOCAL(onerror, "expected type after volatile");
  }

  node = TRY_LOCAL(
      onerror,
      allocator_create(allocator, &g_cubec_expression_type_volatile_type,
                       &(cubec_expression_type_volatile_init_t){
                           .type = type,
                       }));

  /* Set location from 'volatile' keyword to end of type */
  {
    location_t loc = *token_get_location(volatile_token);
    loc.filename = filename;
    loc.end = type->location.end;
    node->super.super.location = loc;
  }

  *position = current;
  return (node_t)node;

onerror:
  allocator_free(allocator, &type);
  allocator_free(allocator, &node);
  return NULL;
}
