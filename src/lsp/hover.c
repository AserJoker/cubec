#include "lsp/hover.h"

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

/* Find the identifier token at a given 0-based position, return the identifier text (malloc'd) or NULL */
static char *find_identifier_at_position(vec_t tokens, size_t line, size_t column) {
  size_t count = vec_get_size(tokens);
  for (size_t i = 0; i < count; i++) {
    token_t tok = vec_get(tokens, i);
    if (token_get_kind(tok) != CUBEC_TOKEN_IDENTIFIER)
      continue;

    location_t *loc = token_get_location(tok);
    if (loc->begin.line == line && loc->end.line == line &&
        column >= loc->begin.column && column < loc->end.column) {
      size_t len = token_get_string_length(tok);
      char *text = (char *)malloc(len + 1);
      if (!text) return NULL;
      memcpy(text, loc->begin.offset, len);
      text[len] = '\0';
      return text;
    }
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

/* Find any token at a given 0-based position (for range extraction) */
static token_t find_token_obj_at_position(vec_t tokens, size_t line, size_t column) {
  size_t count = vec_get_size(tokens);
  for (size_t i = 0; i < count; i++) {
    token_t tok = vec_get(tokens, i);
    location_t *loc = token_get_location(tok);
    if (loc->begin.line == line && loc->end.line == line &&
        column >= loc->begin.column && column < loc->end.column) {
      return tok;
    }
    if (loc->begin.line < line && loc->end.line > line) {
      return tok;
    }
  }
  return NULL;
}

/* Find the symbol for an identifier name by walking all scopes */
static struct symbol *find_symbol_in_scopes(context_t ctx, const char *name) {
  struct symbol *sym = scope_lookup(ctx->global_scope, name);
  if (sym) return sym;

  size_t scope_count = vec_get_size(ctx->all_scopes);
  for (size_t i = 0; i < scope_count; i++) {
    scope_t sc = vec_get(ctx->all_scopes, i);
    sym = scope_lookup(sc, name);
    if (sym) return sym;
  }
  return NULL;
}

/* Build a hover string from a symbol */
static char *build_hover_text(struct symbol *sym) {
  const char *name = sym->name;
  const char *type_name = NULL;

  switch (sym->kind) {
    case SYMBOL_VARIABLE:
      type_name = semantic_type_get_name(sym->variable.type);
      break;
    case SYMBOL_FUNCTION: {
      /* Format function signature: func name(params): return_type */
      semantic_type_t ft = sym->function.type;
      if (ft && semantic_type_get_kind(ft) == TYPE_FUNCTION) {
        type_impl_t impl = semantic_type_get_impl(ft);
        const char *ret = semantic_type_get_name(impl->function.return_type);
        size_t plen = 64 + strlen(name) + (ret ? strlen(ret) : 0);
        /* Add space for parameter type names — params is vec of semantic_type_t */
        size_t pcount = vec_get_size(impl->function.params);
        for (size_t i = 0; i < pcount; i++) {
          semantic_type_t pt = vec_get(impl->function.params, i);
          const char *pname = semantic_type_get_name(pt);
          plen += (pname ? strlen(pname) : 0) + 4;
        }
        char *buf = (char *)malloc(plen);
        if (!buf) return NULL;
        int off = snprintf(buf, plen, "func %s(", name);
        for (size_t i = 0; i < pcount; i++) {
          semantic_type_t pt = vec_get(impl->function.params, i);
          const char *pname = semantic_type_get_name(pt);
          if (i > 0) off += snprintf(buf + off, plen - off, ", ");
          off += snprintf(buf + off, plen - off, "%s", pname ? pname : "?");
        }
        off += snprintf(buf + off, plen - off, "): %s", ret ? ret : "?");
        return buf;
      }
      type_name = semantic_type_get_name(ft);
      break;
    }
    case SYMBOL_TYPE:
      type_name = semantic_type_get_name(sym->type.type);
      break;
    case SYMBOL_ENUM_ITEM:
      type_name = semantic_type_get_name(sym->enum_item.owning_type);
      break;
    case SYMBOL_FIELD:
      type_name = semantic_type_get_name(sym->field.type);
      break;
    case SYMBOL_GENERIC_PARAM:
      type_name = sym->generic_param.value_type
                      ? semantic_type_get_name(sym->generic_param.value_type)
                      : NULL;
      break;
    default:
      break;
  }

  /* Format: "kind name: type" or just "kind name" */
  size_t len = strlen(name) + 32;
  if (type_name) len += strlen(type_name);

  char *buf = (char *)malloc(len);
  if (!buf) return NULL;

  const char *kind_str = "unknown";
  switch (sym->kind) {
    case SYMBOL_VARIABLE:      kind_str = "var"; break;
    case SYMBOL_FUNCTION:      kind_str = "func"; break;
    case SYMBOL_TYPE:          kind_str = "type"; break;
    case SYMBOL_MODULE:        kind_str = "module"; break;
    case SYMBOL_FIELD:         kind_str = "field"; break;
    case SYMBOL_ENUM_ITEM:     kind_str = "enum"; break;
    case SYMBOL_GENERIC_PARAM: kind_str = "typeparam"; break;
  }

  if (type_name) {
    snprintf(buf, len, "%s %s: %s", kind_str, name, type_name);
  } else {
    snprintf(buf, len, "%s %s", kind_str, name);
  }
  return buf;
}

cJSON *lsp_hover_for_position(const char *uri, int line, int character) {
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
  token_t tok = tokens ? find_token_obj_at_position(tokens, cubec_line, cubec_col) : NULL;
  cJSON *result = NULL;

  if (ident) {
    struct symbol *sym = find_symbol_in_scopes(ctx, ident);
    if (sym) {
      char *hover_text = build_hover_text(sym);
      if (hover_text) {
        cJSON *markup = cJSON_CreateObject();
        cJSON_AddStringToObject(markup, "kind", "plaintext");
        cJSON_AddStringToObject(markup, "value", hover_text);

        result = cJSON_CreateObject();
        cJSON_AddItemToObject(result, "contents", markup);
        if (tok) {
          cJSON_AddItemToObject(result, "range", make_range(*token_get_location(tok)));
        }

        free(hover_text);
      }
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
