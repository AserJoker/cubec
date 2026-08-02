#include "cubec/enum_item.h"
#include "core/token.h"
#include "cubec/literal_identifier.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_enum_item_init(cubec_enum_item_t self, allocator_t allocator,
                                  cubec_enum_item_init_t *init) {
  if (!init)
    return;
  node_init_t super_init = {
      .kind = CUBEC_NODE_ENUM_ITEM,
      .parent = NULL,
  };
  super_init.location = init->location;
  g_node_type.init(&self->super, allocator, &super_init);
  self->name = init->name;
  self->type = init->type;
  self->value = init->value;
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
  g_node_type.clone(&self->super, allocator, &another->super);
  self->name = value_clone(allocator, another->name);
  self->type = another->type ? value_clone(allocator, another->type) : NULL;
  self->value = another->value ? value_clone(allocator, another->value) : NULL;
  return;
}

static void _cubec_enum_item_move(cubec_enum_item_t self, allocator_t allocator,
                                  cubec_enum_item_t another) {
  g_node_type.move(&self->super, allocator, &another->super);
  self->name = value_move(allocator, another->name);
  self->type = another->type ? value_move(allocator, another->type) : NULL;
  self->value = another->value ? value_move(allocator, another->value) : NULL;
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

node_t read_enum_item(context_t ctx, vec_t tokens, size_t *position,
                      const char *filename) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;
  node_t name = NULL;
  node_t type_expr = NULL;
  node_t value_expr = NULL;
  cubec_enum_item_t node = NULL;

  /* Parse item name (identifier) */
  name = read_literal_identifier(ctx, tokens, &current, filename);
  if (!name) {
    goto cleanup;
  }

  skip_whitespace(tokens, &current);

  /* Optional ': <type>' */
  token_t colon_token = vec_get(tokens, current);
  if (colon_token && token_is(colon_token, CUBEC_TOKEN_SYMBOL, ":")) {
    current++;
    skip_whitespace(tokens, &current);

    type_expr = read_type_expression_primary(ctx, tokens, &current, filename);
    if (!type_expr) {
      goto cleanup;
    }
    skip_whitespace(tokens, &current);
  }

  /* Optional '= <value>' */
  token_t eq_token = vec_get(tokens, current);
  if (eq_token && token_is(eq_token, CUBEC_TOKEN_SYMBOL, "=")) {
    current++;
    skip_whitespace(tokens, &current);

    value_expr = read_expression_base(ctx, tokens, &current, filename);
    if (!value_expr) {
      goto cleanup;
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
  node = allocator_create(allocator, &g_cubec_enum_item_type, &init);
  if (!node)
    goto cleanup;
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
 *  Factory: create_enum_item
 * -------------------------------------------------------------------------- */

node_t create_enum_item(context_t ctx, location_t loc, const char *name,
                        node_t type, node_t value) {
  allocator_t alloc = ctx->allocator;
  node_t name_node = create_literal_identifier(ctx, loc, name);
  cubec_enum_item_init_t init = {.location = loc,
                                 .parent = NULL,
                                 .name = (node_t)name_node,
                                 .type = type,
                                 .value = value};
  return (node_t)allocator_create(alloc, &g_cubec_enum_item_type, &init);
}
