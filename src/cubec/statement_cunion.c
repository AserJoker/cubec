#include "cubec/statement_cunion.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/node.h"
#include "core/token.h"
#include "core/type.h"
#include "core/vec.h"
#include "cubec/literal_identifier.h"
#include "cubec/node.h"
#include "cubec/struct_field.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_statement_cunion_init(
    cubec_statement_cunion_t self, allocator_t allocator,
    cubec_statement_cunion_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_CUNION,
      .parent = NULL,
  };
  super_init.location = init->location;
  TRY_VOID_LOCAL(onerror, g_node_type.init(&self->super, allocator, &super_init));
  self->name = init->name;
  self->fields = init->fields;
onerror:
  return;
}

static void _cubec_statement_cunion_dispose(
    cubec_statement_cunion_t self, allocator_t allocator) {
  allocator_free(allocator, &self->fields);
  allocator_free(allocator, &self->name);
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_statement_cunion_clone(
    cubec_statement_cunion_t self, allocator_t allocator,
    cubec_statement_cunion_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.clone(&self->super, allocator, &another->super));
  self->name = TRY_LOCAL(onerror, value_clone(allocator, another->name));
  self->fields = TRY_LOCAL(onerror, value_clone(allocator, another->fields));
  return;
onerror:
  return;
}

static void _cubec_statement_cunion_move(
    cubec_statement_cunion_t self, allocator_t allocator,
    cubec_statement_cunion_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.move(&self->super, allocator, &another->super));
  self->name = TRY_LOCAL(onerror, value_move(allocator, another->name));
  self->fields = TRY_LOCAL(onerror, value_move(allocator, another->fields));
  return;
onerror:
  return;
}

type_t g_cubec_statement_cunion_type = {
    .name = "cubec.cubec.statement_cunion",
    .size = sizeof(struct _cubec_statement_cunion_t),
    .init = (type_init_fn_t)_cubec_statement_cunion_init,
    .dispose = (type_dispose_fn_t)_cubec_statement_cunion_dispose,
    .clone = (type_clone_fn_t)_cubec_statement_cunion_clone,
    .move = (type_move_fn_t)_cubec_statement_cunion_move,
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
 *  Parser: read_statement_cunion — cunion <name> { <fields> }
 * -------------------------------------------------------------------------- */

node_t read_statement_cunion(allocator_t allocator, vec_t tokens,
                              size_t *position, const char *filename) {
  size_t current = *position;
  node_t name = NULL;
  vec_t fields = NULL;
  cubec_statement_cunion_t node = NULL;

  /* 1. Expect 'cunion' keyword */
  if (!_is_keyword(tokens, current, "cunion")) {
    return NULL;
  }
  token_t cunion_token = TRY_LOCAL(onerror, vec_get(tokens, current));
  location_t start_location = *token_get_location(cunion_token);
  start_location.filename = filename;
  current++;
  skip_whitespace(tokens, &current);

  /* 2. Parse cunion name (required) */
  name = TRY_LOCAL(cleanup, read_literal_identifier(allocator, tokens, &current, filename));
  if (!name) {
    THROW_LOCAL(cleanup, "expected cunion name after 'cunion'");
  }

  skip_whitespace(tokens, &current);

  /* 3. Expect '{' */
  if (!_is_symbol(tokens, current, "{")) {
    THROW_LOCAL(cleanup, "expected '{' after cunion name");
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 4. Parse fields — semicolon-separated struct_field nodes */
  fields = TRY_LOCAL(cleanup, allocator_create(allocator, &g_vec_type, &(vec_init_t){true}));
  while (!_is_symbol(tokens, current, "}")) {
    node_t field = read_struct_field(allocator, tokens, &current, filename);
    if (!field) {
      break;
    }
    vec_push(fields, field);
    skip_whitespace(tokens, &current);
  }

  /* 5. Expect '}' */
  if (!_is_symbol(tokens, current, "}")) {
    token_t tok = TRY_LOCAL(cleanup, vec_get(tokens, current));
    location_t *loc = token_get_location(tok);
    THROW_LOCAL(cleanup,
                "%s:%" PRIuPTR ":%" PRIuPTR " expected '}' to close cunion",
                filename, loc->begin.line + 1, loc->begin.column);
  }
  token_t close_brace = TRY_LOCAL(cleanup, vec_get(tokens, current));
  current++;

  /* 6. Build location */
  location_t *end_loc = token_get_location(close_brace);
  location_t loc = {
      .begin = start_location.begin,
      .end = end_loc->end,
      .filename = filename,
  };

  cubec_statement_cunion_init_t init = {
      .location = loc,
      .parent = NULL,
      .name = name,
      .fields = fields,
  };
  node = TRY_LOCAL(cleanup, allocator_create(allocator, &g_cubec_statement_cunion_type, &init));
  *position = current;
  return &node->super;

cleanup:
  allocator_free(allocator, &fields);
  allocator_free(allocator, &name);
  allocator_free(allocator, &node);
onerror:
  allocator_free(allocator, &fields);
  allocator_free(allocator, &name);
  allocator_free(allocator, &node);
  return NULL;
}
