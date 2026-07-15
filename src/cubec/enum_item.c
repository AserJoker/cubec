#include "cubec/enum_item.h"
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

static void _cubec_enum_item_init(cubec_enum_item_t self,
                                   allocator_t allocator,
                                   cubec_enum_item_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  node_init_t super_init = {
      .kind = CUBEC_NODE_ENUM_ITEM,
      .parent = NULL,
  };
  super_init.location = init->location;
  TRY_VOID_LOCAL(onerror, g_node_type.init(&self->super, allocator, &super_init));
  self->name = init->name;
  self->type = init->type;
  self->value = init->value;
onerror:
  return;
}

static void _cubec_enum_item_dispose(cubec_enum_item_t self,
                                      allocator_t allocator) {
  allocator_free(allocator, &self->value);
  allocator_free(allocator, &self->type);
  allocator_free(allocator, &self->name);
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_enum_item_clone(cubec_enum_item_t self,
                                    allocator_t allocator,
                                    cubec_enum_item_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.clone(&self->super, allocator, &another->super));
  self->name = TRY_LOCAL(onerror, value_clone(allocator, another->name));
  self->type = another->type ? TRY_LOCAL(onerror, value_clone(allocator, another->type)) : NULL;
  self->value = another->value ? TRY_LOCAL(onerror, value_clone(allocator, another->value)) : NULL;
  return;
onerror:
  return;
}

static void _cubec_enum_item_move(cubec_enum_item_t self,
                                   allocator_t allocator,
                                   cubec_enum_item_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.move(&self->super, allocator, &another->super));
  self->name = TRY_LOCAL(onerror, value_move(allocator, another->name));
  self->type = another->type ? TRY_LOCAL(onerror, value_move(allocator, another->type)) : NULL;
  self->value = another->value ? TRY_LOCAL(onerror, value_move(allocator, another->value)) : NULL;
  return;
onerror:
  return;
}

type_t g_cubec_enum_item_type = {
    .name = "cubec.cubec.enum_item",
    .size = sizeof(struct _cubec_enum_item_t),
    .init = (type_init_fn_t)_cubec_enum_item_init,
    .dispose = (type_dispose_fn_t)_cubec_enum_item_dispose,
    .clone = (type_clone_fn_t)_cubec_enum_item_clone,
    .move = (type_move_fn_t)_cubec_enum_item_move,
};

/* --------------------------------------------------------------------------
 *  Parser: read_enum_item — <identifier> [: <type>] [= <value>]
 * -------------------------------------------------------------------------- */

node_t read_enum_item(allocator_t allocator, vec_t tokens,
                       size_t *position, const char *filename) {
  size_t current = *position;
  node_t name = NULL;
  node_t type_expr = NULL;
  node_t value_expr = NULL;
  cubec_enum_item_t node = NULL;

  /* Parse item name (identifier) */
  name = TRY_LOCAL(cleanup, read_literal_identifier(allocator, tokens, &current, filename));
  if (!name) {
    goto cleanup;
  }

  skip_whitespace(tokens, &current);

  /* Optional ': <type>' */
  token_t colon_token = vec_get(tokens, current);
  if (colon_token && token_is(colon_token, CUBEC_TOKEN_SYMBOL, ":")) {
    location_t colon_loc = *token_get_location(colon_token);
    current++;
    skip_whitespace(tokens, &current);

    type_expr = TRY_LOCAL(cleanup, read_expression_type(allocator, tokens, &current, filename));
    if (!type_expr) {
      THROW_LOCAL(cleanup, "%s:%" PRIuPTR ":%" PRIuPTR " expected type after ':'",
                  filename, colon_loc.begin.line + 1, colon_loc.begin.column);
    }
    skip_whitespace(tokens, &current);
  }

  /* Optional '= <value>' */
  token_t eq_token = vec_get(tokens, current);
  if (eq_token && token_is(eq_token, CUBEC_TOKEN_SYMBOL, "=")) {
    location_t eq_loc = *token_get_location(eq_token);
    current++;
    skip_whitespace(tokens, &current);

    value_expr = TRY_LOCAL(cleanup, read_expression(allocator, tokens, &current, filename));
    if (!value_expr) {
      THROW_LOCAL(cleanup, "%s:%" PRIuPTR ":%" PRIuPTR " expected value after '='",
                  filename, eq_loc.begin.line + 1, eq_loc.begin.column);
    }
    skip_whitespace(tokens, &current);
  }

  /* Build location from name start to last consumed token */
  location_t start_loc = name->location;
  location_t loc = start_loc;
  /* Try to extend to the last consumed token */
  token_t last_token = vec_get(tokens, current > 0 ? current - 1 : 0);
  if (last_token) {
    location_t *end_loc = token_get_location(last_token);
    loc.end = end_loc->end;
  }

  cubec_enum_item_init_t init = {
      .location = loc,
      .parent = NULL,
      .name = name,
      .type = type_expr,
      .value = value_expr,
  };
  node = TRY_LOCAL(cleanup, allocator_create(allocator, &g_cubec_enum_item_type, &init));
  *position = current;
  return &node->super;

cleanup:
  allocator_free(allocator, &value_expr);
  allocator_free(allocator, &type_expr);
  allocator_free(allocator, &name);
  allocator_free(allocator, &node);
  return NULL;
}

/* --------------------------------------------------------------------------
 *  Factory: cubec_ast_create_enum_item
 * -------------------------------------------------------------------------- */

node_t cubec_ast_create_enum_item(allocator_t alloc, location_t loc,
                                  const char *name, node_t type,
                                  node_t value) {
  node_t name_node = (node_t)_make_ident_node(alloc, loc, name);
  cubec_enum_item_init_t init = {.location = loc, .parent = NULL,
                                 .name = name_node, .type = type,
                                 .value = value};
  return (node_t)allocator_create(alloc, &g_cubec_enum_item_type, &init);
}
