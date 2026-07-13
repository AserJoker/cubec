#include "cubec/expression_type_union.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/node.h"
#include "core/token.h"
#include "core/type.h"
#include "core/vec.h"
#include "cubec/generic_param.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include "cubec/union_field.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_expression_type_union_init(
    cubec_expression_type_union_t self, allocator_t allocator,
    cubec_expression_type_union_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  cubec_expression_init_t super_init = {
      .kind = CUBEC_NODE_EXPRESSION_TYPE_UNION,
      .parent = NULL,
  };
  super_init.location = init->location;
  super_init.parent = init->parent;
  TRY_VOID_LOCAL(onerror,
                 g_cubec_expression_type.init(&self->super, allocator, &super_init));
  self->generic_params = init->generic_params;
  self->fields = init->fields;
onerror:
  return;
}

static void _cubec_expression_type_union_dispose(
    cubec_expression_type_union_t self, allocator_t allocator) {
  allocator_free(allocator, &self->fields);
  allocator_free(allocator, &self->generic_params);
  g_cubec_expression_type.dispose(&self->super, allocator);
}

static void _cubec_expression_type_union_clone(
    cubec_expression_type_union_t self, allocator_t allocator,
    cubec_expression_type_union_t another) {
  TRY_VOID_LOCAL(onerror,
                 g_cubec_expression_type.clone(&self->super, allocator, &another->super));
  self->generic_params = another->generic_params
                             ? TRY_LOCAL(onerror, value_clone(allocator, another->generic_params))
                             : NULL;
  self->fields = TRY_LOCAL(onerror, value_clone(allocator, another->fields));
  return;
onerror:
  return;
}

static void _cubec_expression_type_union_move(
    cubec_expression_type_union_t self, allocator_t allocator,
    cubec_expression_type_union_t another) {
  TRY_VOID_LOCAL(onerror,
                 g_cubec_expression_type.move(&self->super, allocator, &another->super));
  self->generic_params = another->generic_params
                             ? TRY_LOCAL(onerror, value_move(allocator, another->generic_params))
                             : NULL;
  self->fields = TRY_LOCAL(onerror, value_move(allocator, another->fields));
  return;
onerror:
  return;
}

type_t g_cubec_expression_type_union_type = {
    .name = "cubec.cubec.expression_type_union",
    .size = sizeof(struct _cubec_expression_type_union_t),
    .init = (type_init_fn_t)_cubec_expression_type_union_init,
    .dispose = (type_dispose_fn_t)_cubec_expression_type_union_dispose,
    .clone = (type_clone_fn_t)_cubec_expression_type_union_clone,
    .move = (type_move_fn_t)_cubec_expression_type_union_move,
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
 *  Internal: parse union body after 'union' keyword consumed
 *            [generic_params] { fields }
 * -------------------------------------------------------------------------- */

node_t read_expression_type_union_body(allocator_t allocator, vec_t tokens,
                                        size_t *position, const char *filename,
                                        location_t start_location) {
  size_t current = *position;
  vec_t generic_params = NULL;
  vec_t fields = NULL;
  cubec_expression_type_union_t node = NULL;

  /* 1. Parse optional generic parameters */
  generic_params = TRY_LOCAL(cleanup, read_generic_params(allocator, tokens, &current, filename));
  if (generic_params) {
    skip_whitespace(tokens, &current);
  }

  /* 2. Expect '{' */
  if (!_is_symbol(tokens, current, "{")) {
    THROW_LOCAL(cleanup, "expected '{' after union");
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 3. Parse fields — comma separated union_field nodes */
  fields = TRY_LOCAL(cleanup, allocator_create(allocator, &g_vec_type, &(vec_init_t){true}));
  while (!_is_symbol(tokens, current, "}")) {
    node_t field = read_union_field(allocator, tokens, &current, filename);
    if (!field) {
      break;
    }
    vec_push(fields, field);
    skip_whitespace(tokens, &current);

    /* Optional comma separator (also allows trailing comma) */
    if (_is_symbol(tokens, current, ",")) {
      current++;
      skip_whitespace(tokens, &current);
    } else {
      break;
    }
  }

  /* 4. Expect '}' */
  if (!_is_symbol(tokens, current, "}")) {
    token_t tok = TRY_LOCAL(cleanup, vec_get(tokens, current));
    location_t *loc = token_get_location(tok);
    THROW_LOCAL(cleanup,
                "%s:%" PRIuPTR ":%" PRIuPTR " expected '}' to close union type",
                filename, loc->begin.line + 1, loc->begin.column);
  }
  token_t close_brace = TRY_LOCAL(cleanup, vec_get(tokens, current));
  current++;

  /* 5. Build location spanning from start to '}' */
  location_t *end_loc = token_get_location(close_brace);
  location_t loc = {
      .begin = start_location.begin,
      .end = end_loc->end,
      .filename = filename,
  };

  cubec_expression_type_union_init_t init = {
      .location = loc,
      .parent = NULL,
      .generic_params = generic_params,
      .fields = fields,
  };
  node = TRY_LOCAL(cleanup, allocator_create(allocator, &g_cubec_expression_type_union_type, &init));
  *position = current;
  return (node_t)&node->super;

cleanup:
  allocator_free(allocator, &fields);
  allocator_free(allocator, &generic_params);
  allocator_free(allocator, &node);
  return NULL;
}

/* --------------------------------------------------------------------------
 *  Parser: read_expression_type_union — entry point for type expressions
 * -------------------------------------------------------------------------- */

node_t read_expression_type_union(allocator_t allocator, vec_t tokens,
                                   size_t *position, const char *filename) {
  size_t current = *position;

  /* Expect 'union' keyword — but NOT 'cunion' */
  if (!_is_keyword(tokens, current, "union")) {
    return NULL;
  }
  token_t union_token = TRY_LOCAL(onerror, vec_get(tokens, current));
  location_t start_location = *token_get_location(union_token);
  start_location.filename = filename;
  current++;
  skip_whitespace(tokens, &current);

  node_t result = read_expression_type_union_body(allocator, tokens, &current, filename, start_location);
  if (result) {
    *position = current;
  }
  return result;

onerror:
  return NULL;
}
