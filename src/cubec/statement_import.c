#include "cubec/statement_import.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/node.h"
#include "core/token.h"
#include "core/type.h"
#include "core/vec.h"
#include "cubec/literal_identifier.h"
#include "cubec/literal_string.h"
#include "cubec/node.h"
#include "cubec/token.h"

static void _cubec_statement_import_init(
    cubec_statement_import_t self, allocator_t allocator,
    cubec_statement_import_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_IMPORT,
      .parent = NULL,
  };
  super_init.location = init->location;
  TRY_VOID_LOCAL(onerror, g_node_type.init(&self->super, allocator, &super_init));
  self->module_name = init->module_name;
  self->alias = init->alias;
  self->path = init->path;
onerror:
  return;
}

static void _cubec_statement_import_dispose(
    cubec_statement_import_t self, allocator_t allocator) {
  allocator_free(allocator, &self->path);
  allocator_free(allocator, &self->alias);
  allocator_free(allocator, &self->module_name);
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_statement_import_clone(
    cubec_statement_import_t self, allocator_t allocator,
    cubec_statement_import_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.clone(&self->super, allocator, &another->super));
  self->module_name = TRY_LOCAL(onerror, value_clone(allocator, another->module_name));
  self->alias = TRY_LOCAL(onerror, value_clone(allocator, another->alias));
  self->path = TRY_LOCAL(onerror, value_clone(allocator, another->path));
  return;
onerror:
  return;
}

static void _cubec_statement_import_move(
    cubec_statement_import_t self, allocator_t allocator,
    cubec_statement_import_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.move(&self->super, allocator, &another->super));
  self->module_name = TRY_LOCAL(onerror, value_move(allocator, another->module_name));
  self->alias = TRY_LOCAL(onerror, value_move(allocator, another->alias));
  self->path = TRY_LOCAL(onerror, value_move(allocator, another->path));
  return;
onerror:
  return;
}

type_t g_cubec_statement_import_type = {
    .name = "cubec.cubec.statement_import",
    .size = sizeof(struct _cubec_statement_import_t),
    .init = (type_init_fn_t)_cubec_statement_import_init,
    .dispose = (type_dispose_fn_t)_cubec_statement_import_dispose,
    .clone = (type_clone_fn_t)_cubec_statement_import_clone,
    .move = (type_move_fn_t)_cubec_statement_import_move,
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

node_t read_statement_import(allocator_t allocator, vec_t tokens,
                             size_t *position, const char *filename) {
  size_t current = *position;
  cubec_statement_import_t node = NULL;
  node_t module_name = NULL;
  node_t alias = NULL;
  node_t path = NULL;
  location_t start_location = {0};

  /* Expect 'import' keyword */
  if (!_is_keyword(tokens, current, "import")) {
    return NULL;
  }
  token_t import_token = TRY_LOCAL(onerror, vec_get(tokens, current));
  start_location = *token_get_location(import_token);
  start_location.filename = filename;
  current++;

  skip_whitespace(tokens, &current);

  /* Parse module name (required identifier) */
  module_name = TRY_LOCAL(cleanup, read_literal_identifier(allocator, tokens, &current, filename));
  if (!module_name) {
    THROW_LOCAL(cleanup, "expected module name after 'import'");
  }

  skip_whitespace(tokens, &current);

  /* Check for optional 'as' keyword */
  if (_is_keyword(tokens, current, "as")) {
    current++;
    skip_whitespace(tokens, &current);

    /* Parse alias (required after 'as') */
    alias = TRY_LOCAL(cleanup, read_literal_identifier(allocator, tokens, &current, filename));
    if (!alias) {
      THROW_LOCAL(cleanup, "expected alias after 'as'");
    }

    skip_whitespace(tokens, &current);
  }

  /* Expect 'from' keyword */
  if (!_is_keyword(tokens, current, "from")) {
    token_t tok = TRY_LOCAL(cleanup, vec_get(tokens, current));
    location_t *loc = token_get_location(tok);
    THROW_LOCAL(cleanup,
                "%s:%" PRIuPTR ":%" PRIuPTR " expected 'from' in import statement",
                filename, loc->begin.line + 1, loc->begin.column);
  }
  current++;

  skip_whitespace(tokens, &current);

  /* Parse module path (required string literal) */
  path = TRY_LOCAL(cleanup, read_literal_string(allocator, tokens, &current, filename));
  if (!path) {
    THROW_LOCAL(cleanup, "expected string path after 'from'");
  }

  skip_whitespace(tokens, &current);

  /* Expect semicolon */
  token_t semi = TRY_LOCAL(cleanup, vec_get(tokens, current));
  if (!semi || !token_is(semi, CUBEC_TOKEN_SYMBOL, ";")) {
    location_t *loc = token_get_location(semi);
    THROW_LOCAL(cleanup,
                "%s:%" PRIuPTR ":%" PRIuPTR " expected ';' after import statement",
                filename, loc->begin.line + 1, loc->begin.column);
  }
  current++;

  /* Build location spanning from 'import' to ';' */
  location_t *end_loc = token_get_location(semi);
  location_t loc = {
      .begin = start_location.begin,
      .end = end_loc->end,
      .filename = filename,
  };

  cubec_statement_import_init_t init = {
      .location = loc,
      .parent = NULL,
      .module_name = module_name,
      .alias = alias,
      .path = path,
  };
  node = TRY_LOCAL(cleanup, allocator_create(allocator, &g_cubec_statement_import_type, &init));
  *position = current;
  return &node->super;

cleanup:
  allocator_free(allocator, &path);
  allocator_free(allocator, &alias);
  allocator_free(allocator, &module_name);
  allocator_free(allocator, &node);
onerror:
  allocator_free(allocator, &path);
  allocator_free(allocator, &alias);
  allocator_free(allocator, &module_name);
  allocator_free(allocator, &node);
  return NULL;
}
