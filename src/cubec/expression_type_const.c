#include "cubec/expression_type_const.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/token.h"
#include "cubec/expression.h"
#include "cubec/node.h"
#include "cubec/token.h"

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_expression_type_const_init(
    cubec_expression_type_const_t self, allocator_t allocator,
    cubec_expression_type_const_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  cubec_expression_init_t super_init = {
      .kind = CUBEC_NODE_EXPRESSION_TYPE_CONST,
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

static void _cubec_expression_type_const_dispose(
    cubec_expression_type_const_t self, allocator_t allocator) {
  allocator_free(allocator, &self->type);
  g_cubec_expression_type.dispose(&self->super, allocator);
}

static void _cubec_expression_type_const_clone(
    cubec_expression_type_const_t self, allocator_t allocator,
    cubec_expression_type_const_t another) {
  TRY_VOID_LOCAL(
      cleanup,
      g_cubec_expression_type.clone(&self->super, allocator, &another->super));
  self->type = TRY_LOCAL(cleanup, value_clone(allocator, another->type));
  return;

cleanup:
  allocator_free(allocator, &self->type);
}

static void _cubec_expression_type_const_move(
    cubec_expression_type_const_t self, allocator_t allocator,
    cubec_expression_type_const_t another) {
  TRY_VOID_LOCAL(
      cleanup,
      g_cubec_expression_type.move(&self->super, allocator, &another->super));
  self->type = TRY_LOCAL(cleanup, value_move(allocator, another->type));
  return;

cleanup:
  allocator_free(allocator, &self->type);
}

type_t g_cubec_expression_type_const_type = {
    .name = "cubec.cubec.expression_type_const",
    .size = sizeof(struct _cubec_expression_type_const_t),
    .init = (type_init_fn_t)_cubec_expression_type_const_init,
    .dispose = (type_dispose_fn_t)_cubec_expression_type_const_dispose,
    .clone = (type_clone_fn_t)_cubec_expression_type_const_clone,
    .move = (type_move_fn_t)_cubec_expression_type_const_move,
};

/* --------------------------------------------------------------------------
 *  Parser: read_expression_type_const
 * -------------------------------------------------------------------------- */

node_t read_expression_type_const(allocator_t allocator, vec_t tokens,
                                  size_t *position, const char *filename) {
  size_t current = *position;
  cubec_expression_type_const_t node = NULL;
  node_t type = NULL;

  /* Expect 'const' keyword */
  token_t const_token = vec_get(tokens, current);
  if (!const_token || !token_is(const_token, CUBEC_TOKEN_KEYWORD, "const")) {
    return NULL;
  }
  current++;

  /* Skip whitespace after 'const' */
  skip_whitespace(tokens, &current);

  /* Parse the underlying type using read_type_expression_primary.
   * Ternary type expressions are not allowed directly as const base type;
   * use type_group to wrap them: const (a ? b : c). */
  type = TRY_LOCAL(onerror,
                   read_type_expression_primary(allocator, tokens, &current, filename));
  if (!type) {
    THROW_LOCAL(onerror, "expected type after const");
  }

  node = TRY_LOCAL(
      onerror,
      allocator_create(allocator, &g_cubec_expression_type_const_type,
                       &(cubec_expression_type_const_init_t){
                           .type = type,
                       }));

  /* Set location from 'const' keyword to end of type */
  {
    location_t loc = *token_get_location(const_token);
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
