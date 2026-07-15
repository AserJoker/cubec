#include "cubec/union_field.h"
#include "cubec/ast_factory_internal.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/node.h"
#include "core/token.h"
#include "core/type.h"
#include "cubec/ast_factory.h"
#include "cubec/expression.h"
#include "cubec/literal_identifier.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_union_field_init(cubec_union_field_t self,
                                      allocator_t allocator,
                                      cubec_union_field_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  node_init_t super_init = {
      .kind = CUBEC_NODE_UNION_FIELD,
      .parent = NULL,
  };
  super_init.location = init->location;
  TRY_VOID_LOCAL(onerror, g_node_type.init(&self->super, allocator, &super_init));
  self->name = init->name;
  self->type = init->type;
onerror:
  return;
}

static void _cubec_union_field_dispose(cubec_union_field_t self,
                                         allocator_t allocator) {
  allocator_free(allocator, &self->type);
  allocator_free(allocator, &self->name);
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_union_field_clone(cubec_union_field_t self,
                                       allocator_t allocator,
                                       cubec_union_field_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.clone(&self->super, allocator, &another->super));
  self->name = TRY_LOCAL(onerror, value_clone(allocator, another->name));
  self->type = TRY_LOCAL(onerror, value_clone(allocator, another->type));
  return;
onerror:
  return;
}

static void _cubec_union_field_move(cubec_union_field_t self,
                                      allocator_t allocator,
                                      cubec_union_field_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.move(&self->super, allocator, &another->super));
  self->name = TRY_LOCAL(onerror, value_move(allocator, another->name));
  self->type = TRY_LOCAL(onerror, value_move(allocator, another->type));
  return;
onerror:
  return;
}

type_t g_cubec_union_field_type = {
    .name = "cubec.cubec.union_field",
    .size = sizeof(struct _cubec_union_field_t),
    .init = (type_init_fn_t)_cubec_union_field_init,
    .dispose = (type_dispose_fn_t)_cubec_union_field_dispose,
    .clone = (type_clone_fn_t)_cubec_union_field_clone,
    .move = (type_move_fn_t)_cubec_union_field_move,
};

/* --------------------------------------------------------------------------
 *  Parser: read_union_field — <identifier> : <type> ;
 * -------------------------------------------------------------------------- */

node_t read_union_field(allocator_t allocator, vec_t tokens,
                          size_t *position, const char *filename) {
  size_t current = *position;
  node_t name = NULL;
  node_t type_expr = NULL;
  cubec_union_field_t node = NULL;

  /* Parse field name (identifier) */
  name = TRY_LOCAL(cleanup, read_literal_identifier(allocator, tokens, &current, filename));
  if (!name) {
    goto cleanup;
  }

  /* Expect ':' */
  skip_whitespace(tokens, &current);
  token_t colon_token = TRY_LOCAL(cleanup, vec_get(tokens, current));
  if (!token_is(colon_token, CUBEC_TOKEN_SYMBOL, ":")) {
    goto cleanup;
  }
  location_t colon_loc = *token_get_location(colon_token);
  current++;
  skip_whitespace(tokens, &current);

  /* Parse type expression */
  type_expr = TRY_LOCAL(cleanup, read_expression_type(allocator, tokens, &current, filename));
  if (!type_expr) {
    THROW_LOCAL(cleanup, "%s:%" PRIuPTR ":%" PRIuPTR " expected type after ':'",
                filename, colon_loc.begin.line + 1, colon_loc.begin.column);
  }

  /* Expect ';' */
  skip_whitespace(tokens, &current);
  token_t semi = TRY_LOCAL(cleanup, vec_get(tokens, current));
  if (!token_is(semi, CUBEC_TOKEN_SYMBOL, ";")) {
    location_t *loc = token_get_location(semi);
    THROW_LOCAL(cleanup,
                "%s:%" PRIuPTR ":%" PRIuPTR " expected ';' after union field",
                filename, loc->begin.line + 1, loc->begin.column);
  }
  current++;

  /* Build location from name start to ';' end */
  location_t start_loc = name->location;
  location_t *end_loc = token_get_location(semi);
  location_t loc = {
      .begin = start_loc.begin,
      .end = end_loc->end,
      .filename = filename,
  };

  cubec_union_field_init_t init = {
      .location = loc,
      .parent = NULL,
      .name = name,
      .type = type_expr,
  };
  node = TRY_LOCAL(cleanup, allocator_create(allocator, &g_cubec_union_field_type, &init));
  *position = current;
  return &node->super;

cleanup:
  allocator_free(allocator, &type_expr);
  allocator_free(allocator, &name);
  allocator_free(allocator, &node);
  return NULL;
}

/* --------------------------------------------------------------------------
 *  Factory: cubec_ast_create_union_field
 * -------------------------------------------------------------------------- */

node_t cubec_ast_create_union_field(allocator_t alloc, location_t loc,
                                    const char *name, node_t type) {
  node_t name_node = (node_t)_make_ident_node(alloc, loc, name);
  cubec_union_field_init_t init = {.location = loc, .parent = NULL,
                                   .name = name_node, .type = type};
  return (node_t)allocator_create(alloc, &g_cubec_union_field_type, &init);
}
