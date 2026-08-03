#include "cubec/struct_field.h"
#include "core/token.h"
#include "cubec/literal_identifier.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_struct_field_init(cubec_struct_field_t self,
                                     allocator_t allocator,
                                     cubec_struct_field_init_t *init) {
  if (!init)
    return;
  node_init_t super_init = {
      .kind = CUBEC_NODE_STRUCT_FIELD,
      .parent = NULL,
  };
  super_init.location = init->location;
  g_node_type.init(&self->super, allocator, &super_init);
  self->is_pub = init->is_pub;
  self->name = init->name;
  self->type = init->type;
}

static void _cubec_struct_field_dispose(cubec_struct_field_t self,
                                        allocator_t allocator) {
  allocator_free(allocator, &self->type);
  allocator_free(allocator, &self->name);
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_struct_field_clone(cubec_struct_field_t self,
                                      allocator_t allocator,
                                      cubec_struct_field_t another) {
  g_node_type.clone(&self->super, allocator, &another->super);
  self->is_pub = another->is_pub;
  self->name = value_clone(allocator, another->name);
  self->type = value_clone(allocator, another->type);
  return;
}

static void _cubec_struct_field_move(cubec_struct_field_t self,
                                     allocator_t allocator,
                                     cubec_struct_field_t another) {
  g_node_type.move(&self->super, allocator, &another->super);
  self->is_pub = another->is_pub;
  self->name = value_move(allocator, another->name);
  self->type = value_move(allocator, another->type);
  return;
}

type_t g_cubec_struct_field_type = {
    .name = "cubec.cubec.struct_field",
    .size = sizeof(struct _cubec_struct_field_t),
    .init = (type_init_fn_t)_cubec_struct_field_init,
    .dispose = (type_dispose_fn_t)_cubec_struct_field_dispose,
    .clone = (type_clone_fn_t)_cubec_struct_field_clone,
    .move = (type_move_fn_t)_cubec_struct_field_move,
};

/* --------------------------------------------------------------------------
 *  Helper: check keyword
 * -------------------------------------------------------------------------- */

static bool _is_keyword(vec_t tokens, size_t position, const char *keyword) {
  token_t token = vec_get(tokens, position);
  if (!token)
    return false;
  if (token_get_kind(token) != CUBEC_TOKEN_KEYWORD)
    return false;
  return location_is(token_get_location(token), keyword);
}

/* --------------------------------------------------------------------------
 *  Parser: read_struct_field — [pub] <identifier> : <type> ;
 * -------------------------------------------------------------------------- */

node_t read_struct_field(context_t ctx, vec_t tokens, size_t *position,
                         const char *filename) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;
  bool is_pub = false;
  node_t name = NULL;
  node_t type_expr = NULL;
  cubec_struct_field_t node = NULL;

  /* Optional 'pub' modifier */
  if (_is_keyword(tokens, current, "pub")) {
    is_pub = true;
    current++;
    skip_whitespace(tokens, &current);
  }

  /* Parse field name (identifier) */
  name = read_literal_identifier(ctx, tokens, &current, filename);
  if (!name) {
    /* Not a struct field — no identifier found (possibly after 'pub' too) */
    goto cleanup;
  }

  /* Expect ':' */
  skip_whitespace(tokens, &current);
  token_t colon_token = vec_get(tokens, current);
  if (!token_is(colon_token, CUBEC_TOKEN_SYMBOL, ":")) {
    /* Not a struct field — identifier not followed by ':' */
    goto cleanup;
  }
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

  /* Build location — from 'pub' or name start to ';' end */
  location_t start_loc;
  if (is_pub) {
    token_t pub_token = vec_get(tokens, *position);
    start_loc = *token_get_location(pub_token);
  } else {
    start_loc = name->location;
  }
  location_t *end_loc = token_get_location(semi);
  location_t loc = {
      .begin = start_loc.begin,
      .end = end_loc->end,
      .filename = filename,
  };

  cubec_struct_field_init_t init = {
      .location = loc,
      .parent = NULL,
      .is_pub = is_pub,
      .name = name,
      .type = type_expr,
  };
  node = allocator_create(allocator, &g_cubec_struct_field_type, &init);
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
 *  Factory: create_struct_field
 * -------------------------------------------------------------------------- */

node_t create_struct_field(context_t ctx, location_t loc, const char *name,
                           node_t type, bool is_pub) {
  allocator_t alloc = ctx->allocator;
  node_t name_node = create_literal_identifier(ctx, loc, name);
  cubec_struct_field_init_t init = {
      .location = loc,
      .parent = NULL,
      .is_pub = is_pub,
      .name = name_node,
      .type = type,
  };
  return (node_t)allocator_create(alloc, &g_cubec_struct_field_type, &init);
}

void write_struct_field(writer_t writer, node_t node) {
  cubec_struct_field_t field = (cubec_struct_field_t)node;
  if (field->is_pub) {
    writer_append(writer, "pub ");
  }
  write_expression(writer, field->name);
  writer_append(writer, ": ");
  write_expression(writer, field->type);
  writer_append(writer, ";");
}
