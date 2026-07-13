#include "cubec/expression_type_enum.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/node.h"
#include "core/token.h"
#include "core/type.h"
#include "core/vec.h"
#include "cubec/enum_item.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_expression_type_enum_init(
    cubec_expression_type_enum_t self, allocator_t allocator,
    cubec_expression_type_enum_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  cubec_expression_init_t super_init = {
      .kind = CUBEC_NODE_EXPRESSION_TYPE_ENUM,
      .parent = NULL,
  };
  super_init.location = init->location;
  super_init.parent = init->parent;
  TRY_VOID_LOCAL(onerror,
                 g_cubec_expression_type.init(&self->super, allocator, &super_init));
  self->items = init->items;
onerror:
  return;
}

static void _cubec_expression_type_enum_dispose(
    cubec_expression_type_enum_t self, allocator_t allocator) {
  allocator_free(allocator, &self->items);
  g_cubec_expression_type.dispose(&self->super, allocator);
}

static void _cubec_expression_type_enum_clone(
    cubec_expression_type_enum_t self, allocator_t allocator,
    cubec_expression_type_enum_t another) {
  TRY_VOID_LOCAL(onerror,
                 g_cubec_expression_type.clone(&self->super, allocator, &another->super));
  self->items = TRY_LOCAL(onerror, value_clone(allocator, another->items));
  return;
onerror:
  return;
}

static void _cubec_expression_type_enum_move(
    cubec_expression_type_enum_t self, allocator_t allocator,
    cubec_expression_type_enum_t another) {
  TRY_VOID_LOCAL(onerror,
                 g_cubec_expression_type.move(&self->super, allocator, &another->super));
  self->items = TRY_LOCAL(onerror, value_move(allocator, another->items));
  return;
onerror:
  return;
}

type_t g_cubec_expression_type_enum_type = {
    .name = "cubec.cubec.expression_type_enum",
    .size = sizeof(struct _cubec_expression_type_enum_t),
    .init = (type_init_fn_t)_cubec_expression_type_enum_init,
    .dispose = (type_dispose_fn_t)_cubec_expression_type_enum_dispose,
    .clone = (type_clone_fn_t)_cubec_expression_type_enum_clone,
    .move = (type_move_fn_t)_cubec_expression_type_enum_move,
};

/* --------------------------------------------------------------------------
 *  Helper: check keyword / symbol
 * -------------------------------------------------------------------------- */

static bool _is_keyword(vec_t tokens, size_t position, const char *keyword) {
  token_t token = vec_get(tokens, position);
  if (!token) return false;
  if (token_get_kind(token) != CUBEC_TOKEN_KEYWORD) return false;
  return location_is(token_get_location(token), keyword);
}

static bool _is_symbol(vec_t tokens, size_t position, const char *symbol) {
  token_t token = vec_get(tokens, position);
  if (!token) return false;
  return token_is(token, CUBEC_TOKEN_SYMBOL, symbol);
}

/* --------------------------------------------------------------------------
 *  Internal: parse enum body after 'enum' keyword consumed
 *            { items }
 * -------------------------------------------------------------------------- */

node_t read_expression_type_enum_body(allocator_t allocator, vec_t tokens,
                                       size_t *position, const char *filename,
                                       location_t start_location) {
  size_t current = *position;
  vec_t items = NULL;
  cubec_expression_type_enum_t node = NULL;

  /* 1. Expect '{' */
  if (!_is_symbol(tokens, current, "{")) {
    THROW_LOCAL(cleanup, "expected '{' after enum");
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 2. Parse items — comma separated enum_item nodes */
  items = TRY_LOCAL(cleanup, allocator_create(allocator, &g_vec_type, &(vec_init_t){true}));
  while (!_is_symbol(tokens, current, "}")) {
    node_t item = read_enum_item(allocator, tokens, &current, filename);
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
    token_t tok = TRY_LOCAL(cleanup, vec_get(tokens, current));
    location_t *loc = token_get_location(tok);
    THROW_LOCAL(cleanup,
                "%s:%" PRIuPTR ":%" PRIuPTR " expected '}' to close enum type",
                filename, loc->begin.line + 1, loc->begin.column);
  }
  token_t close_brace = TRY_LOCAL(cleanup, vec_get(tokens, current));
  current++;

  /* 4. Build location spanning from start to '}' */
  location_t *end_loc = token_get_location(close_brace);
  location_t loc = {
      .begin = start_location.begin,
      .end = end_loc->end,
      .filename = filename,
  };

  cubec_expression_type_enum_init_t init = {
      .location = loc,
      .parent = NULL,
      .items = items,
  };
  node = TRY_LOCAL(cleanup, allocator_create(allocator, &g_cubec_expression_type_enum_type, &init));
  *position = current;
  return (node_t)&node->super;

cleanup:
  allocator_free(allocator, &items);
  allocator_free(allocator, &node);
  return NULL;
}

/* --------------------------------------------------------------------------
 *  Parser: read_expression_type_enum — entry point for type expressions
 * -------------------------------------------------------------------------- */

node_t read_expression_type_enum(allocator_t allocator, vec_t tokens,
                                  size_t *position, const char *filename) {
  size_t current = *position;

  /* Expect 'enum' keyword */
  if (!_is_keyword(tokens, current, "enum")) {
    return NULL;
  }
  token_t enum_token = TRY_LOCAL(onerror, vec_get(tokens, current));
  location_t start_location = *token_get_location(enum_token);
  start_location.filename = filename;
  current++;
  skip_whitespace(tokens, &current);

  node_t result = read_expression_type_enum_body(allocator, tokens, &current, filename, start_location);
  if (result) {
    *position = current;
  }
  return result;

onerror:
  return NULL;
}
