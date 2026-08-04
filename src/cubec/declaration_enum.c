#include "cubec/declaration_enum.h"
#include "core/emit_context.h"
#include "core/token.h"
#include "core/writer.h"
#include "cubec/enum_item.h"
#include "cubec/node_error.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_declaration_enum_init(cubec_declaration_enum_t self,
                                        allocator_t allocator,
                                        cubec_declaration_enum_init_t *init) {
  if (!init)
    return;
  cubec_expression_init_t super_init = {
      .kind = CUBEC_NODE_DECLARATION_ENUM,
      .parent = NULL,
  };
  super_init.location = init->location;
  super_init.parent = init->parent;
  g_cubec_expression_type.init(&self->super, allocator, &super_init);
  self->items = init->items;
}

static void _cubec_declaration_enum_dispose(cubec_declaration_enum_t self,
                                           allocator_t allocator) {
  allocator_free(allocator, &self->items);
  g_cubec_expression_type.dispose(&self->super, allocator);
}

static void _cubec_declaration_enum_clone(cubec_declaration_enum_t self,
                                         allocator_t allocator,
                                         cubec_declaration_enum_t another) {
  g_cubec_expression_type.clone(&self->super, allocator, &another->super);
  self->items = value_clone(allocator, another->items);
  return;
}

static void _cubec_declaration_enum_move(cubec_declaration_enum_t self,
                                        allocator_t allocator,
                                        cubec_declaration_enum_t another) {
  g_cubec_expression_type.move(&self->super, allocator, &another->super);
  self->items = value_move(allocator, another->items);
  return;
}

type_t g_cubec_declaration_enum_type = {
    .name = "cubec.cubec.declaration_enum",
    .size = sizeof(struct _cubec_declaration_enum_t),
    .init = (type_init_fn_t)_cubec_declaration_enum_init,
    .dispose = (type_dispose_fn_t)_cubec_declaration_enum_dispose,
    .clone = (type_clone_fn_t)_cubec_declaration_enum_clone,
    .move = (type_move_fn_t)_cubec_declaration_enum_move,
};

/* --------------------------------------------------------------------------
 *  Helper: check keyword / symbol
 * -------------------------------------------------------------------------- */

static bool _is_keyword(vec_t tokens, size_t position, const char *keyword) {
  token_t token = vec_get(tokens, position);
  if (!token)
    return false;
  if (token_get_kind(token) != CUBEC_TOKEN_KEYWORD)
    return false;
  return location_is(token_get_location(token), keyword);
}

static bool _is_symbol(vec_t tokens, size_t position, const char *symbol) {
  token_t token = vec_get(tokens, position);
  if (!token)
    return false;
  return token_is(token, CUBEC_TOKEN_SYMBOL, symbol);
}

/* --------------------------------------------------------------------------
 *  Internal: parse enum body after 'enum' keyword consumed
 *            { items }
 * -------------------------------------------------------------------------- */

node_t read_declaration_enum_body(context_t ctx, vec_t tokens, size_t *position,
                                 const char *filename,
                                 location_t start_location) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;
  vec_t items = NULL;
  cubec_declaration_enum_t node = NULL;

  /* 1. Expect '{' */
  if (!_is_symbol(tokens, current, "{")) {
    goto cleanup;
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 2. Parse items — comma separated enum_item nodes */
  items = allocator_create(allocator, &g_vec_type, &(vec_init_t){true});
  while (!_is_symbol(tokens, current, "}")) {
    node_t item = read_enum_item(ctx, tokens, &current, filename);
    if (node_is_error(item))
      goto onerror;
    if (!item) {
      break;
    }
    vec_push(items, item);
    skip_whitespace(tokens, &current);

    /* Optional comma separator (also allows trailing comma) */
    if (_is_symbol(tokens, current, ",")) {
      current++;
      skip_whitespace(tokens, &current);
    } else {
      break;
    }
  }

  /* 3. Expect '}' */
  if (!_is_symbol(tokens, current, "}")) {
    goto cleanup;
  }
  token_t close_brace = vec_get(tokens, current);
  current++;

  /* 4. Build location spanning from start to '}' */
  location_t *end_loc = token_get_location(close_brace);
  location_t loc = {
      .begin = start_location.begin,
      .end = end_loc->end,
      .filename = filename,
  };

  cubec_declaration_enum_init_t init = {
      .location = loc,
      .parent = NULL,
      .items = items,
  };
  node = allocator_create(allocator, &g_cubec_declaration_enum_type, &init);
  *position = current;
  return (node_t)&node->super;

cleanup:
  allocator_free(allocator, &items);
  allocator_free(allocator, &node);
  return NULL;
onerror:
  allocator_free(allocator, &items);
  allocator_free(allocator, &node);
  return create_error(ctx, start_location);
}

/* --------------------------------------------------------------------------
 *  Parser: read_declaration_enum — entry point for type expressions
 * -------------------------------------------------------------------------- */

node_t read_declaration_enum(context_t ctx, vec_t tokens, size_t *position,
                            const char *filename) {
  size_t current = *position;

  /* Expect 'enum' keyword */
  if (!_is_keyword(tokens, current, "enum")) {
    return NULL;
  }
  token_t enum_token = vec_get(tokens, current);
  location_t start_location = *token_get_location(enum_token);
  start_location.filename = filename;
  current++;
  skip_whitespace(tokens, &current);

  node_t result = read_declaration_enum_body(ctx, tokens, &current, filename,
                                            start_location);
  if (node_is_error(result))
    return result;
  if (result) {
    *position = current;
    return result;
  }

  diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, start_location,
                       "invalid enum type expression");
  return create_error(ctx, start_location);
}

/* --------------------------------------------------------------------------
 *  Factory: create_declaration_enum
 * -------------------------------------------------------------------------- */

node_t create_declaration_enum(context_t ctx, location_t loc, vec_t items) {
  allocator_t alloc = ctx->allocator;
  cubec_declaration_enum_init_t init = {
      .location = loc,
      .parent = NULL,
      .items = items,
  };
  return (node_t)allocator_create(alloc, &g_cubec_declaration_enum_type, &init);
}

/* --------------------------------------------------------------------------
 *  Writer: write_declaration_enum
 * -------------------------------------------------------------------------- */

void write_declaration_enum(writer_t writer, node_t node) {
  cubec_declaration_enum_t en = (cubec_declaration_enum_t)node;
  writer_append(writer, "enum {");
  if (vec_get_size(en->items)) {
    writer_newline(writer, 1);
    for (size_t i = 0; i < vec_get_size(en->items); i++) {
      if (i != 0) {
        writer_append(writer, ",");
        writer_newline(writer, 0);
      }
      write_enum_item(writer, vec_get(en->items, i));
    }
    writer_append(writer, ",");
    writer_newline(writer, -1);
  }
  writer_append(writer, "}");
}

void emit_declaration_enum(emit_context_t ctx, node_t node) {
  cubec_declaration_enum_t en = (cubec_declaration_enum_t)node;
  recover_comments_to(ctx, node->location.begin.offset);
  emit_keyword(ctx, "enum");
  emit_space(ctx);
  emit_symbol(ctx, "{");
  if (vec_get_size(en->items)) {
    emit_newline(ctx);
    emit_indent(ctx, +1);
    for (size_t i = 0; i < vec_get_size(en->items); i++) {
      recover_comments_to(ctx, ((node_t)vec_get(en->items, i))->location.begin.offset);
      emit_enum_item(ctx, vec_get(en->items, i));
      emit_symbol(ctx, ",");
      emit_newline(ctx);
    }
    emit_indent(ctx, -1);
  }
  emit_symbol(ctx, "}");
}
