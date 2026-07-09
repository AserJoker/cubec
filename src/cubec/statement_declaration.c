#include "cubec/statement_declaration.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/node.h"
#include "core/token.h"
#include "core/type.h"
#include "core/vec.h"
#include "cubec/declaration_variable.h"
#include "cubec/node.h"
#include "cubec/token.h"

static void _cubec_statement_declaration_init(
    cubec_statement_declaration_t self, allocator_t allocator,
    cubec_statement_declaration_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_DECLARATION,
      .parent = NULL,
  };
  super_init.location = init->location;
  TRY_VOID_LOCAL(onerror, g_node_type.init(&self->super, allocator, &super_init));
  self->declarators = init->declarators;
onerror:
  return;
}

static void _cubec_statement_declaration_dispose(
    cubec_statement_declaration_t self, allocator_t allocator) {
  allocator_free(allocator, &self->declarators);
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_statement_declaration_clone(
    cubec_statement_declaration_t self, allocator_t allocator,
    cubec_statement_declaration_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.clone(&self->super, allocator, &another->super));
  self->declarators = TRY_LOCAL(onerror, value_clone(allocator, another->declarators));
  return;
onerror:
  return;
}

static void _cubec_statement_declaration_move(
    cubec_statement_declaration_t self, allocator_t allocator,
    cubec_statement_declaration_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.move(&self->super, allocator, &another->super));
  self->declarators = TRY_LOCAL(onerror, value_move(allocator, another->declarators));
  return;
onerror:
  return;
}

type_t g_cubec_statement_declaration_type = {
    .name = "cubec.cubec.statement_declaration",
    .size = sizeof(struct _cubec_statement_declaration_t),
    .init = (type_init_fn_t)_cubec_statement_declaration_init,
    .dispose = (type_dispose_fn_t)_cubec_statement_declaration_dispose,
    .clone = (type_clone_fn_t)_cubec_statement_declaration_clone,
    .move = (type_move_fn_t)_cubec_statement_declaration_move,
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

node_t read_statement_declaration(allocator_t allocator, vec_t tokens,
                                  size_t *position, const char *filename) {
  size_t current = *position;
  cubec_statement_declaration_t node = NULL;
  vec_t declarators = NULL;
  location_t start_location = {0};

  /* Expect 'var' keyword */
  if (!_is_keyword(tokens, current, "var")) {
    return NULL;
  }
  token_t var_token = TRY_LOCAL(onerror, vec_get(tokens, current));
  start_location = *token_get_location(var_token);
  start_location.filename = filename;
  current++;

  skip_whitespace(tokens, &current);

  /* Create declarators vec with auto_dispose */
  declarators = TRY_LOCAL(onerror, allocator_create(allocator, &g_vec_type, &(vec_init_t){true}));

  /* Parse first declarator (required) */
  node_t first_declarator = TRY_LOCAL(cleanup, read_declaration_variable(allocator, tokens, &current, filename));
  if (!first_declarator) {
    THROW_LOCAL(cleanup, "expected variable declarator after 'var'");
  }
  vec_push(declarators, first_declarator);

  skip_whitespace(tokens, &current);

  /* Parse additional declarators (optional, comma-separated) */
  while (true) {
    token_t comma = TRY_LOCAL(cleanup, vec_get(tokens, current));
    if (!token_is(comma, CUBEC_TOKEN_SYMBOL, ",")) {
      break;
    }
    current++;
    skip_whitespace(tokens, &current);

    node_t declarator = TRY_LOCAL(cleanup, read_declaration_variable(allocator, tokens, &current, filename));
    if (!declarator) {
      THROW_LOCAL(cleanup, "expected variable declarator after ','");
    }
    vec_push(declarators, declarator);
    skip_whitespace(tokens, &current);
  }

  /* Expect semicolon */
  token_t semi = TRY_LOCAL(cleanup, vec_get(tokens, current));
  if (!token_is(semi, CUBEC_TOKEN_SYMBOL, ";")) {
    location_t *loc = token_get_location(semi);
    THROW_LOCAL(cleanup,
                "%s:%" PRIuPTR ":%" PRIuPTR " expected ';' after declaration statement",
                filename, loc->begin.line + 1, loc->begin.column);
  }
  current++;

  /* Build location spanning from 'var' to semicolon */
  location_t *end_loc = token_get_location(semi);
  location_t loc = {
      .begin = start_location.begin,
      .end = end_loc->end,
      .filename = filename,
  };

  cubec_statement_declaration_init_t init = {
      .location = loc,
      .parent = NULL,
      .declarators = declarators,
  };
  node = TRY_LOCAL(cleanup, allocator_create(allocator, &g_cubec_statement_declaration_type, &init));
  *position = current;
  return &node->super;

cleanup:
  allocator_free(allocator, &declarators);
  allocator_free(allocator, &node);
onerror:
  allocator_free(allocator, &declarators);
  allocator_free(allocator, &node);
  return NULL;
}