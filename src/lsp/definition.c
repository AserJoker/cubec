#include "lsp/definition.h"

#include "core/allocator.h"
#include "core/position.h"
#include "core/token.h"
#include "core/vec.h"
#include "cubec/program.h"
#include "cubec/token.h"
#include <string.h>

/* Convert a cubec position (0-based) to LSP Position (0-based) — identity */
static cJSON *make_position(size_t line, size_t column) {
  cJSON *pos = cJSON_CreateObject();
  cJSON_AddNumberToObject(pos, "line", line);
  cJSON_AddNumberToObject(pos, "character", column);
  return pos;
}

/* Convert a cubec location to LSP Range */
static cJSON *make_range(location_t loc) {
  cJSON *range = cJSON_CreateObject();
  cJSON_AddItemToObject(range, "start", make_position(loc.begin.line, loc.begin.column));
  cJSON_AddItemToObject(range, "end", make_position(loc.end.line, loc.end.column));
  return range;
}

/* Build a LSP Location object from a cubec location */
static cJSON *make_location(const char *fallback_uri, location_t loc) {
  cJSON *result = cJSON_CreateObject();
  const char *uri = loc.filename ? loc.filename : fallback_uri;
  cJSON_AddStringToObject(result, "uri", uri);
  cJSON_AddItemToObject(result, "range", make_range(loc));
  return result;
}

/* Find the token at a given 0-based position, return the identifier text (malloc'd) or NULL */
static char *find_identifier_at_position(vec_t tokens, size_t line, size_t column) {
  size_t count = vec_get_size(tokens);
  for (size_t i = 0; i < count; i++) {
    token_t tok = vec_get(tokens, i);
    if (token_get_kind(tok) != CUBEC_TOKEN_IDENTIFIER)
      continue;

    location_t *loc = token_get_location(tok);
    /* Check if position falls within this token's span */
    if (loc->begin.line == line && loc->end.line == line &&
        column >= loc->begin.column && column < loc->end.column) {
      /* Extract identifier text using offset pointers */
      size_t len = token_get_string_length(tok);
      char *text = (char *)malloc(len + 1);
      if (!text) return NULL;
      memcpy(text, loc->begin.offset, len);
      text[len] = '\0';
      return text;
    }
    /* Multi-line tokens are unlikely for identifiers, but handle anyway */
    if (loc->begin.line < line && loc->end.line > line) {
      size_t len = token_get_string_length(tok);
      char *text = (char *)malloc(len + 1);
      if (!text) return NULL;
      memcpy(text, loc->begin.offset, len);
      text[len] = '\0';
      return text;
    }
  }
  return NULL;
}

/* Find the symbol for an identifier name by walking all scopes */
static struct symbol *find_symbol_in_scopes(context_t ctx, const char *name) {
  /* Try global scope first */
  struct symbol *sym = scope_lookup(ctx->global_scope, name);
  if (sym) return sym;

  /* Try all collected scopes */
  size_t scope_count = vec_get_size(ctx->all_scopes);
  for (size_t i = 0; i < scope_count; i++) {
    scope_t sc = vec_get(ctx->all_scopes, i);
    sym = scope_lookup(sc, name);
    if (sym) return sym;
  }
  return NULL;
}

cJSON *lsp_definition_for_position(const char *uri, int line, int character) {
  /* We need the document source from handler's store.
   * Since handler.c owns the doc store and exposes doc_get internally,
   * we must access it. Use a thin accessor added to handler. */
  extern const char *lsp_handler_doc_get(const char *uri);

  const char *source = lsp_handler_doc_get(uri);
  if (!source) return cJSON_CreateNull();

  /* LSP and cubec both use 0-based positions — no conversion needed */
  size_t cubec_line = (size_t)line;
  size_t cubec_col = (size_t)character;

  /* Lex + parse + check */
  allocator_t allocator = create_allocator(NULL, NULL);
  context_t ctx = context_create(allocator);
  ctx->current_file = uri;
  source_cache_load(ctx->sources, uri, source, false);

  vec_t tokens = resolve_token_list(ctx, uri, source);
  node_t program = NULL;
  if (tokens) {
    size_t position = 0;
    program = read_program_node(ctx, tokens, &position, uri);
  }
  if (program) {
    context_check_program(ctx, program);
  }

  /* Find the identifier at the position */
  char *ident = tokens ? find_identifier_at_position(tokens, cubec_line, cubec_col) : NULL;
  cJSON *result = NULL;

  if (ident) {
    struct symbol *sym = find_symbol_in_scopes(ctx, ident);
    if (sym) {
      result = make_location(uri, sym->location);
    }
    free(ident);
  }

  /* Cleanup */
  if (program) allocator_free(allocator, &program);
  if (tokens) allocator_free(allocator, &tokens);
  context_dispose(ctx);
  delete_allocator(allocator);

  return result ? result : cJSON_CreateNull();
}
