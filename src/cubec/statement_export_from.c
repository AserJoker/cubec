#include "cubec/statement_export_from.h"
#include "cubec/ast_factory_internal.h"
#include "cubec/ast_factory.h"
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

/* ===== lifecycle ===== */

static void _cubec_statement_export_from_init(
    cubec_statement_export_from_t self, allocator_t allocator,
    cubec_statement_export_from_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_EXPORT_FROM,
      .parent = NULL,
  };
  super_init.location = init->location;
  TRY_VOID_LOCAL(onerror, g_node_type.init(&self->super, allocator, &super_init));
  self->path = init->path;
  self->is_star = init->is_star;
  self->names = init->names;
onerror:
  return;
}

static void _cubec_statement_export_from_dispose(
    cubec_statement_export_from_t self, allocator_t allocator) {
  allocator_free(allocator, &self->names);
  allocator_free(allocator, &self->path);
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_statement_export_from_clone(
    cubec_statement_export_from_t self, allocator_t allocator,
    cubec_statement_export_from_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.clone(&self->super, allocator, &another->super));
  self->path = TRY_LOCAL(onerror, value_clone(allocator, another->path));
  self->is_star = another->is_star;
  self->names = TRY_LOCAL(onerror, value_clone(allocator, another->names));
  return;
onerror:
  return;
}

static void _cubec_statement_export_from_move(
    cubec_statement_export_from_t self, allocator_t allocator,
    cubec_statement_export_from_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.move(&self->super, allocator, &another->super));
  self->path = TRY_LOCAL(onerror, value_move(allocator, another->path));
  self->is_star = another->is_star;
  self->names = TRY_LOCAL(onerror, value_move(allocator, another->names));
  return;
onerror:
  return;
}

type_t g_cubec_statement_export_from_type = {
    .name = "cubec.cubec.statement_export_from",
    .size = sizeof(struct _cubec_statement_export_from_t),
    .init = (type_init_fn_t)_cubec_statement_export_from_init,
    .dispose = (type_dispose_fn_t)_cubec_statement_export_from_dispose,
    .clone = (type_clone_fn_t)_cubec_statement_export_from_clone,
    .move = (type_move_fn_t)_cubec_statement_export_from_move,
};

/* ===== helpers ===== */

static bool _is_keyword(vec_t tokens, size_t position, const char *keyword) {
  token_t token = vec_get(tokens, position);
  if (!token) return false;
  if (token_get_kind(token) != CUBEC_TOKEN_KEYWORD) return false;
  return location_is(token_get_location(token), keyword);
}

/* ===== parser ===== */

node_t read_statement_export_from(allocator_t allocator, vec_t tokens,
                                  size_t *position, const char *filename) {
  size_t current = *position;
  cubec_statement_export_from_t node = NULL;
  node_t path = NULL;
  vec_t names = NULL;
  location_t start_location = {0};

  /* Expect 'export' keyword */
  if (!_is_keyword(tokens, current, "export")) {
    return NULL;
  }
  token_t export_token = TRY_LOCAL(onerror, vec_get(tokens, current));
  start_location = *token_get_location(export_token);
  start_location.filename = filename;
  current++;

  skip_whitespace(tokens, &current);

  /* Check next token: '*' or '{' */
  token_t next = TRY_LOCAL(onerror, vec_get(tokens, current));
  if (!next) return NULL;

  bool is_star = false;
  if (token_is(next, CUBEC_TOKEN_SYMBOL, "*")) {
    /* export * from "path" */
    is_star = true;
    current++;
  } else if (token_is(next, CUBEC_TOKEN_SYMBOL, "{")) {
    /* export { name1, name2 } from "path" */
    is_star = false;
    current++;
    skip_whitespace(tokens, &current);

    names = TRY_LOCAL(cleanup, allocator_create(allocator, &g_vec_type,
                         &(vec_init_t){.auto_dispose = true}));
    /* Read comma-separated identifiers */
    while (true) {
      skip_whitespace(tokens, &current);
      node_t name = TRY_LOCAL(cleanup, read_literal_identifier(allocator, tokens, &current, filename));
      if (!name) {
        THROW_LOCAL(cleanup, "expected identifier in export list");
      }
      vec_push(names, name);

      skip_whitespace(tokens, &current);
      token_t tok = TRY_LOCAL(cleanup, vec_get(tokens, current));
      if (token_is(tok, CUBEC_TOKEN_SYMBOL, "}")) {
        current++;
        break;
      }
      if (token_is(tok, CUBEC_TOKEN_SYMBOL, ",")) {
        current++;
        continue;
      }
      THROW_LOCAL(cleanup, "expected ',' or '}' in export list");
    }
  } else {
    /* export followed by something else (struct/func/etc.) — not our node */
    return NULL;
  }

  skip_whitespace(tokens, &current);

  /* Expect 'from' keyword */
  if (!_is_keyword(tokens, current, "from")) {
    token_t tok = TRY_LOCAL(cleanup, vec_get(tokens, current));
    location_t *loc = token_get_location(tok);
    THROW_LOCAL(cleanup,
                "%s:%" PRIuPTR ":%" PRIuPTR " expected 'from' in export statement",
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
                "%s:%" PRIuPTR ":%" PRIuPTR " expected ';' after export statement",
                filename, loc->begin.line + 1, loc->begin.column);
  }
  current++;

  /* Build location spanning from 'export' to ';' */
  location_t *end_loc = token_get_location(semi);
  location_t loc = {
      .begin = start_location.begin,
      .end = end_loc->end,
      .filename = filename,
  };

  cubec_statement_export_from_init_t init = {
      .location = loc,
      .parent = NULL,
      .path = path,
      .is_star = is_star,
      .names = names,
  };
  node = TRY_LOCAL(cleanup, allocator_create(allocator, &g_cubec_statement_export_from_type, &init));
  *position = current;
  return &node->super;

cleanup:
  allocator_free(allocator, &names);
  allocator_free(allocator, &path);
  allocator_free(allocator, &node);
onerror:
  allocator_free(allocator, &names);
  allocator_free(allocator, &path);
  allocator_free(allocator, &node);
  return NULL;
}
