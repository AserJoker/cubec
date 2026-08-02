#include "cubec/statement_export_from.h"
#include "core/token.h"
#include "core/vec.h"
#include "core/writer.h"
#include "cubec/literal_identifier.h"
#include "cubec/literal_string.h"
#include "cubec/node_error.h"
#include "cubec/token.h"

/* ===== lifecycle ===== */

static void
_cubec_statement_export_from_init(cubec_statement_export_from_t self,
                                  allocator_t allocator,
                                  cubec_statement_export_from_init_t *init) {
  if (!init)
    return;
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_EXPORT_FROM,
      .parent = NULL,
  };
  super_init.location = init->location;
  g_node_type.init(&self->super, allocator, &super_init);
  self->path = init->path;
  self->is_star = init->is_star;
  self->names = init->names;
}

static void
_cubec_statement_export_from_dispose(cubec_statement_export_from_t self,
                                     allocator_t allocator) {
  allocator_free(allocator, &self->names);
  allocator_free(allocator, &self->path);
  g_node_type.dispose(&self->super, allocator);
}

static void
_cubec_statement_export_from_clone(cubec_statement_export_from_t self,
                                   allocator_t allocator,
                                   cubec_statement_export_from_t another) {
  g_node_type.clone(&self->super, allocator, &another->super);
  self->path = value_clone(allocator, another->path);
  self->is_star = another->is_star;
  self->names = value_clone(allocator, another->names);
  return;
}

static void
_cubec_statement_export_from_move(cubec_statement_export_from_t self,
                                  allocator_t allocator,
                                  cubec_statement_export_from_t another) {
  g_node_type.move(&self->super, allocator, &another->super);
  self->path = value_move(allocator, another->path);
  self->is_star = another->is_star;
  self->names = value_move(allocator, another->names);
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
  if (!token)
    return false;
  if (token_get_kind(token) != CUBEC_TOKEN_KEYWORD)
    return false;
  return location_is(token_get_location(token), keyword);
}

/* ===== parser ===== */

node_t read_statement_export_from(context_t ctx, vec_t tokens, size_t *position,
                                  const char *filename) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;
  cubec_statement_export_from_t node = NULL;
  node_t path = NULL;
  vec_t names = NULL;
  location_t start_location = {0};

  /* Expect 'export' keyword */
  if (!_is_keyword(tokens, current, "export")) {
    return NULL;
  }
  token_t export_token = vec_get(tokens, current);
  start_location = *token_get_location(export_token);
  start_location.filename = filename;
  current++;

  skip_whitespace(tokens, &current);

  /* Check next token: '*' or '{' */
  token_t next = vec_get(tokens, current);
  if (!next)
    return NULL;

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

    names = allocator_create(allocator, &g_vec_type,
                             &(vec_init_t){.auto_dispose = true});
    /* Read comma-separated identifiers */
    while (true) {
      skip_whitespace(tokens, &current);
      node_t name = read_literal_identifier(ctx, tokens, &current, filename);
      if (!name) {
        goto onerror;
      }
      vec_push(names, name);

      skip_whitespace(tokens, &current);
      token_t tok = vec_get(tokens, current);
      if (token_is(tok, CUBEC_TOKEN_SYMBOL, "}")) {
        current++;
        break;
      }
      if (token_is(tok, CUBEC_TOKEN_SYMBOL, ",")) {
        current++;
        continue;
      }
      goto onerror;
    }
  } else {
    /* export followed by something else (struct/func/etc.) — not our node */
    return NULL;
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
  node =
      allocator_create(allocator, &g_cubec_statement_export_from_type, &init);
  *position = current;
  return &node->super;

onerror:
  diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, start_location,
                       "invalid export from statement");
  ctx->error_count++;
  allocator_free(allocator, &names);
  allocator_free(allocator, &path);
  allocator_free(allocator, &node);
  return create_error(ctx, start_location);
}

/* --------------------------------------------------------------------------
 *  Factory: create_statement_export_from
 * -------------------------------------------------------------------------- */

node_t create_statement_export_from(context_t ctx, location_t loc, node_t path,
                                    bool is_star, vec_t names) {
  allocator_t alloc = ctx->allocator;
  cubec_statement_export_from_init_t init = {
      .location = loc,
      .parent = NULL,
      .path = path,
      .is_star = is_star,
      .names = names,
  };
  return (node_t)allocator_create(alloc, &g_cubec_statement_export_from_type,
                                  &init);
}

void write_statement_export_from(writer_t writer, node_t node) {
  cubec_statement_export_from_t export_from =
      (cubec_statement_export_from_t)node;
  writer_append(writer, "export ");
  if (export_from->is_star) {
    writer_append(writer, "*");
  } else {
    writer_append(writer, "{");
    for (size_t i = 0; i < vec_get_size(export_from->names); i++) {
      if (i > 0) {
        writer_append(writer, ", ");
      }
      write_literal_identifier(writer, vec_get(export_from->names, i));
    }
    writer_append(writer, "}");
  }
  writer_append(writer, " from ");
  write_literal_string(writer, export_from->path);
  writer_append(writer, ";");
  writer_newline(writer, 0);
}