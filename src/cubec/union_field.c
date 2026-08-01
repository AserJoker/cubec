#include "cubec/union_field.h"
#include "core/token.h"
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
  if (!init)
    return;
  node_init_t super_init = {
      .kind = CUBEC_NODE_UNION_FIELD,
      .parent = NULL,
  };
  super_init.location = init->location;
  g_node_type.init(&self->super, allocator, &super_init);
  self->name = init->name;
  self->type = init->type;
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
  g_node_type.clone(&self->super, allocator, &another->super);
  self->name = value_clone(allocator, another->name);
  self->type = value_clone(allocator, another->type);
  return;
}

static void _cubec_union_field_move(cubec_union_field_t self,
                                    allocator_t allocator,
                                    cubec_union_field_t another) {
  g_node_type.move(&self->super, allocator, &another->super);
  self->name = value_move(allocator, another->name);
  self->type = value_move(allocator, another->type);
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

node_t read_union_field(context_t ctx, vec_t tokens, size_t *position,
                        const char *filename) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;
  node_t name = NULL;
  node_t type_expr = NULL;
  cubec_union_field_t node = NULL;

  /* Parse field name (identifier) */
  name = read_literal_identifier(ctx, tokens, &current, filename);
  if (!name) {
    goto cleanup;
  }

  /* Expect ':' */
  skip_whitespace(tokens, &current);
  token_t colon_token = vec_get(tokens, current);
  if (!token_is(colon_token, CUBEC_TOKEN_SYMBOL, ":")) {
    goto cleanup;
  }
  (void)0; /* colon consumed */
  current++;
  skip_whitespace(tokens, &current);

  /* Parse type expression */
  type_expr = read_expression_type(ctx, tokens, &current, filename);
  if (!type_expr) {
    goto cleanup;
  }

  /* Expect ';' */
  skip_whitespace(tokens, &current);
  token_t semi = vec_get(tokens, current);
  if (!token_is(semi, CUBEC_TOKEN_SYMBOL, ";")) {
    goto cleanup;
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
  node = allocator_create(allocator, &g_cubec_union_field_type, &init);
  if (!node)
    goto cleanup;
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

node_t cubec_ast_create_union_field(context_t ctx, location_t loc,
                                    const char *name, node_t type) {
  allocator_t alloc = ctx->allocator;
  node_t name_node = cubec_ast_create_identifier(ctx, loc, name);
  cubec_union_field_init_t init = {
      .location = loc,
      .parent = NULL,
      .name = name_node,
      .type = type,
  };
  return (node_t)allocator_create(alloc, &g_cubec_union_field_type, &init);
}
