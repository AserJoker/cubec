#include "cubec/statement_import.h"
#include "core/emit_context.h"
#include "core/token.h"
#include "cubec/literal_identifier.h"
#include "cubec/literal_string.h"
#include "cubec/node_error.h"
#include "cubec/token.h"

static void _cubec_statement_import_init(cubec_statement_import_t self,
                                         allocator_t allocator,
                                         cubec_statement_import_init_t *init) {
  if (!init)
    return;
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_IMPORT,
      .parent = NULL,
  };
  super_init.location = init->location;
  g_node_type.init(&self->super, allocator, &super_init);
  self->module_name = init->module_name;
  self->path = init->path;
}

static void _cubec_statement_import_dispose(cubec_statement_import_t self,
                                            allocator_t allocator) {
  allocator_free(allocator, &self->path);
  allocator_free(allocator, &self->module_name);
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_statement_import_clone(cubec_statement_import_t self,
                                          allocator_t allocator,
                                          cubec_statement_import_t another) {
  g_node_type.clone(&self->super, allocator, &another->super);
  self->module_name = alloc_clone(allocator, another->module_name);
  self->path = alloc_clone(allocator, another->path);
  return;
}

static void _cubec_statement_import_move(cubec_statement_import_t self,
                                         allocator_t allocator,
                                         cubec_statement_import_t another) {
  g_node_type.move(&self->super, allocator, &another->super);
  self->module_name = alloc_move(allocator, another->module_name);
  self->path = alloc_move(allocator, another->path);
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
  if (!token)
    return false;
  if (token_get_kind(token) != CUBEC_TOKEN_KEYWORD)
    return false;
  return location_is(token_get_location(token), keyword);
}

node_t read_statement_import(context_t ctx, vec_t tokens, size_t *position,
                             const char *filename) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;
  cubec_statement_import_t node = NULL;
  node_t module_name = NULL;
  node_t path = NULL;
  location_t start_location = {0};

  /* Expect 'import' keyword */
  if (!_is_keyword(tokens, current, "import")) {
    return NULL;
  }
  token_t import_token = vec_get(tokens, current);
  start_location = *token_get_location(import_token);
  start_location.filename = filename;
  current++;

  skip_whitespace(tokens, &current);

  /* Parse module name (required identifier) */
  module_name = read_literal_identifier(ctx, tokens, &current, filename);
  if (!module_name) {
    goto onerror;
  }

  skip_whitespace(tokens, &current);

  /* Expect 'from' keyword */
  if (!_is_keyword(tokens, current, "from")) {
    goto onerror;
  }
  current++;

  skip_whitespace(tokens, &current);

  /* Parse module path (required string literal) */
  path = read_literal_string(ctx, tokens, &current, filename);
  if (!path) {
    goto onerror;
  }

  skip_whitespace(tokens, &current);

  /* Expect semicolon */
  token_t semi = vec_get(tokens, current);
  if (!semi || !token_is(semi, CUBEC_TOKEN_SYMBOL, ";")) {
    goto onerror;
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
      .path = path,
  };
  node = allocator_create(allocator, &g_cubec_statement_import_type, &init);
  *position = current;
  return &node->super;

onerror:
  diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, start_location,
                       "invalid import statement");
  allocator_free(allocator, &path);
  allocator_free(allocator, &module_name);
  allocator_free(allocator, &node);
  return create_error(ctx, start_location);
}

node_t create_statement_import(context_t ctx, location_t loc,
                               const char *module_name, const char *path) {
  allocator_t alloc = ctx->allocator;
  node_t mod_node =
      (module_name) ? create_literal_identifier(ctx, loc, module_name) : NULL;
  node_t path_node = (path) ? create_literal_identifier(ctx, loc, path) : NULL;
  cubec_statement_import_init_t init = {.location = loc,
                                        .parent = NULL,
                                        .module_name = mod_node,
                                        .path = path_node};
  return (node_t)allocator_create(alloc, &g_cubec_statement_import_type, &init);
}

void emit_statement_import(emit_context_t ctx, node_t node) {
  cubec_statement_import_t import = (cubec_statement_import_t)node;
  recover_comments_to(ctx, node->location.begin.offset);
  emit_keyword(ctx, "import");
  emit_space(ctx);
  emit_literal_identifier(ctx, import->module_name);
  emit_space(ctx);
  emit_keyword(ctx, "from");
  emit_space(ctx);
  emit_literal_string(ctx, import->path);
  emit_symbol(ctx, ";");
}