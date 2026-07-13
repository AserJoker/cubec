#include "cubec/statement_declaration_type.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/node.h"
#include "core/token.h"
#include "core/type.h"
#include "core/vec.h"
#include "cubec/expression.h"
#include "cubec/generic_param.h"
#include "cubec/literal_identifier.h"
#include "cubec/node.h"
#include "cubec/token.h"

static void _cubec_statement_declaration_type_init(
    cubec_statement_declaration_type_t self, allocator_t allocator,
    cubec_statement_declaration_type_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_DECLARATION_TYPE,
      .parent = NULL,
  };
  super_init.location = init->location;
  TRY_VOID_LOCAL(onerror, g_node_type.init(&self->super, allocator, &super_init));
  self->is_export = init->is_export;
  self->is_builtin = init->is_builtin;
  self->name = init->name;
  self->params = init->params;
  self->type_value = init->type_value;
onerror:
  return;
}

static void _cubec_statement_declaration_type_dispose(
    cubec_statement_declaration_type_t self, allocator_t allocator) {
  allocator_free(allocator, &self->type_value);
  allocator_free(allocator, &self->params);
  allocator_free(allocator, &self->name);
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_statement_declaration_type_clone(
    cubec_statement_declaration_type_t self, allocator_t allocator,
    cubec_statement_declaration_type_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.clone(&self->super, allocator, &another->super));
  self->is_export = another->is_export;
  self->is_builtin = another->is_builtin;
  self->name = TRY_LOCAL(onerror, value_clone(allocator, another->name));
  self->params = TRY_LOCAL(onerror, value_clone(allocator, another->params));
  self->type_value = another->type_value
                         ? TRY_LOCAL(onerror, value_clone(allocator, another->type_value))
                         : NULL;
  return;
onerror:
  return;
}

static void _cubec_statement_declaration_type_move(
    cubec_statement_declaration_type_t self, allocator_t allocator,
    cubec_statement_declaration_type_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.move(&self->super, allocator, &another->super));
  self->is_export = another->is_export;
  self->is_builtin = another->is_builtin;
  self->name = TRY_LOCAL(onerror, value_move(allocator, another->name));
  self->params = TRY_LOCAL(onerror, value_move(allocator, another->params));
  self->type_value = another->type_value
                         ? TRY_LOCAL(onerror, value_move(allocator, another->type_value))
                         : NULL;
  return;
onerror:
  return;
}

type_t g_cubec_statement_decltype = {
    .name = "cubec.cubec.statement_declaration_type",
    .size = sizeof(struct _cubec_statement_declaration_type_t),
    .init = (type_init_fn_t)_cubec_statement_declaration_type_init,
    .dispose = (type_dispose_fn_t)_cubec_statement_declaration_type_dispose,
    .clone = (type_clone_fn_t)_cubec_statement_declaration_type_clone,
    .move = (type_move_fn_t)_cubec_statement_declaration_type_move,
};

/**
 * @brief Check if a token is a specific keyword.
 */
static bool _is_keyword(vec_t tokens, size_t position, const char *keyword) {
  token_t token = vec_get(tokens, position);
  if (!token) return false;
  if (token_get_kind(token) != CUBEC_TOKEN_KEYWORD) return false;
  return location_is(token_get_location(token), keyword);
}

node_t read_statement_declaration_type(allocator_t allocator, vec_t tokens,
                                       size_t *position, const char *filename) {
  size_t current = *position;
  cubec_statement_declaration_type_t node = NULL;
  node_t name = NULL;
  vec_t params = NULL;
  node_t type_value = NULL;
  location_t start_location = {0};
  bool is_export = false;
  bool is_builtin = false;

  /* 1. Parse optional modifiers: export / builtin */
  while (true) {
    if (_is_keyword(tokens, current, "export")) {
      if (is_export) THROW_LOCAL(onerror, "duplicate 'export' modifier");
      is_export = true;
      if (start_location.begin.offset == 0) {
        token_t tok = TRY_LOCAL(onerror, vec_get(tokens, current));
        start_location = *token_get_location(tok);
        start_location.filename = filename;
      }
      current++;
      skip_whitespace(tokens, &current);
    } else if (_is_keyword(tokens, current, "builtin")) {
      if (is_builtin) THROW_LOCAL(onerror, "duplicate 'builtin' modifier");
      is_builtin = true;
      if (start_location.begin.offset == 0) {
        token_t tok = TRY_LOCAL(onerror, vec_get(tokens, current));
        start_location = *token_get_location(tok);
        start_location.filename = filename;
      }
      current++;
      skip_whitespace(tokens, &current);
    } else {
      break;
    }
  }

  /* 2. Expect 'type' keyword */
  if (!_is_keyword(tokens, current, "type")) {
    return NULL;
  }
  token_t type_token = TRY_LOCAL(onerror, vec_get(tokens, current));
  if (start_location.begin.offset == 0) {
    start_location = *token_get_location(type_token);
    start_location.filename = filename;
  }
  current++;

  skip_whitespace(tokens, &current);

  /* 3. Parse type alias name (required) */
  name = TRY_LOCAL(cleanup, read_literal_identifier(allocator, tokens, &current, filename));
  if (!name) {
    THROW_LOCAL(cleanup, "expected type name after 'type'");
  }

  skip_whitespace(tokens, &current);

  /* 4. Parse optional generic parameters */
  params = TRY_LOCAL(cleanup, read_generic_params(allocator, tokens, &current, filename));

  if (params) {
    skip_whitespace(tokens, &current);
  }

  /* 5. Parse optional '= type_expression' (required for non-builtin, absent for builtin) */
  if (!is_builtin) {
    token_t eq = TRY_LOCAL(cleanup, vec_get(tokens, current));
    if (!eq || !token_is(eq, CUBEC_TOKEN_SYMBOL, "=")) {
      location_t *loc = token_get_location(eq);
      THROW_LOCAL(cleanup,
                  "%s:%" PRIuPTR ":%" PRIuPTR " expected '=' after type name",
                  filename, loc->begin.line + 1, loc->begin.column);
    }
    current++;

    skip_whitespace(tokens, &current);

    /* Parse type expression */
    type_value = TRY_LOCAL(cleanup, read_expression_type(allocator, tokens, &current, filename));
    if (!type_value) {
      THROW_LOCAL(cleanup, "expected type expression after '='");
    }

    skip_whitespace(tokens, &current);
  }

  /* 6. Expect ';' */
  token_t semi = TRY_LOCAL(cleanup, vec_get(tokens, current));
  if (!semi || !token_is(semi, CUBEC_TOKEN_SYMBOL, ";")) {
    location_t *loc = token_get_location(semi);
    THROW_LOCAL(cleanup,
                "%s:%" PRIuPTR ":%" PRIuPTR " expected ';' after type declaration statement",
                filename, loc->begin.line + 1, loc->begin.column);
  }
  current++;

  /* 7. Build location spanning from first modifier or 'type' to ';' */
  location_t *end_loc = token_get_location(semi);
  location_t loc = {
      .begin = start_location.begin,
      .end = end_loc->end,
      .filename = filename,
  };

  cubec_statement_declaration_type_init_t init = {
      .location = loc,
      .parent = NULL,
      .is_export = is_export,
      .is_builtin = is_builtin,
      .name = name,
      .params = params,
      .type_value = type_value,
  };
  node = TRY_LOCAL(cleanup, allocator_create(allocator, &g_cubec_statement_decltype, &init));
  *position = current;
  return &node->super;

cleanup:
  allocator_free(allocator, &type_value);
  allocator_free(allocator, &params);
  allocator_free(allocator, &name);
  allocator_free(allocator, &node);
onerror:
  allocator_free(allocator, &type_value);
  allocator_free(allocator, &params);
  allocator_free(allocator, &name);
  allocator_free(allocator, &node);
  return NULL;
}
