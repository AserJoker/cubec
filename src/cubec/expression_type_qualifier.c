#include "cubec/expression_type_qualifier.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/token.h"
#include "cubec/ast_factory.h"
#include "cubec/ast_factory_internal.h"
#include "cubec/expression.h"
#include "cubec/node.h"
#include "cubec/token.h"

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_expression_type_qualifier_init(
    cubec_expression_type_qualifier_t self, allocator_t allocator,
    cubec_expression_type_qualifier_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  cubec_expression_init_t super_init = {
      .kind = CUBEC_NODE_EXPRESSION_TYPE_QUALIFIER,
      .parent = NULL,
  };
  super_init.location = init->location;
  super_init.parent = init->parent;
  TRY_VOID_LOCAL(onerror,
                 g_cubec_expression_type.init(&self->super, allocator, &super_init));
  self->type = init->type;
  self->is_volatile = init->is_volatile;
onerror:
  return;
}

static void _cubec_expression_type_qualifier_dispose(
    cubec_expression_type_qualifier_t self, allocator_t allocator) {
  allocator_free(allocator, &self->type);
  g_cubec_expression_type.dispose(&self->super, allocator);
}

static void _cubec_expression_type_qualifier_clone(
    cubec_expression_type_qualifier_t self, allocator_t allocator,
    cubec_expression_type_qualifier_t another) {
  TRY_VOID_LOCAL(
      cleanup,
      g_cubec_expression_type.clone(&self->super, allocator, &another->super));
  self->type = TRY_LOCAL(cleanup, value_clone(allocator, another->type));
  self->is_volatile = another->is_volatile;
  return;

cleanup:
  allocator_free(allocator, &self->type);
}

static void _cubec_expression_type_qualifier_move(
    cubec_expression_type_qualifier_t self, allocator_t allocator,
    cubec_expression_type_qualifier_t another) {
  TRY_VOID_LOCAL(
      cleanup,
      g_cubec_expression_type.move(&self->super, allocator, &another->super));
  self->type = TRY_LOCAL(cleanup, value_move(allocator, another->type));
  self->is_volatile = another->is_volatile;
  return;

cleanup:
  allocator_free(allocator, &self->type);
}

type_t g_cubec_expression_type_qualifier_type = {
    .name = "cubec.cubec.expression_type_qualifier",
    .size = sizeof(struct _cubec_expression_type_qualifier_t),
    .init = (type_init_fn_t)_cubec_expression_type_qualifier_init,
    .dispose = (type_dispose_fn_t)_cubec_expression_type_qualifier_dispose,
    .clone = (type_clone_fn_t)_cubec_expression_type_qualifier_clone,
    .move = (type_move_fn_t)_cubec_expression_type_qualifier_move,
};

/* --------------------------------------------------------------------------
 *  Parser: read_expression_type_qualifier
 * -------------------------------------------------------------------------- */

node_t read_expression_type_qualifier(allocator_t allocator, vec_t tokens,
                                      size_t *position, const char *filename) {
  size_t current = *position;
  cubec_expression_type_qualifier_t node = NULL;
  node_t type = NULL;
  bool is_volatile = false;

  /* Expect 'const' or 'volatile' keyword */
  token_t keyword_token = vec_get(tokens, current);
  if (!keyword_token || token_get_kind(keyword_token) != CUBEC_TOKEN_KEYWORD) {
    return NULL;
  }
  if (token_is(keyword_token, CUBEC_TOKEN_KEYWORD, "const")) {
    is_volatile = false;
  } else if (token_is(keyword_token, CUBEC_TOKEN_KEYWORD, "volatile")) {
    is_volatile = true;
  } else {
    return NULL;
  }
  current++;

  /* Skip whitespace after keyword */
  skip_whitespace(tokens, &current);

  /* Parse the underlying type using read_expression_type (greedy).
   * The qualifier greedily consumes the full type expression,
   * including ternary: const a ? b : c → const(ternary(a, b, c)).
   * Use grouping for the alternative: (const a) ? b : c → ternary(const(a), b, c).
   * Namespace access binds tighter: const std::vec::Vec → const(std::vec::Vec). */
  type = TRY_LOCAL(onerror,
                   read_expression_type(allocator, tokens, &current, filename));
  if (!type) {
    THROW_LOCAL(onerror, "expected type after const/volatile");
  }

  node = TRY_LOCAL(
      onerror,
      allocator_create(allocator, &g_cubec_expression_type_qualifier_type,
                       &(cubec_expression_type_qualifier_init_t){
                           .type = type,
                           .is_volatile = is_volatile,
                       }));

  /* Set location from keyword to end of type */
  {
    location_t loc = *token_get_location(keyword_token);
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

/* --------------------------------------------------------------------------
 *  Factory: cubec_ast_create_type_qualifier
 * -------------------------------------------------------------------------- */

node_t cubec_ast_create_type_qualifier(allocator_t alloc, location_t loc,
                                       node_t base, bool is_volatile) {
  cubec_expression_type_qualifier_init_t init = {
      .location = loc, .parent = NULL, .type = base,
      .is_volatile = is_volatile};
  return (node_t)allocator_create(
      alloc, &g_cubec_expression_type_qualifier_type, &init);
}
