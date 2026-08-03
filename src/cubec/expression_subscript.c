#include "cubec/expression_subscript.h"
#include "core/token.h"
#include "core/writer.h"
#include "cubec/expression.h"
#include "cubec/node_error.h"
#include "cubec/token.h"


/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void
_cubec_expression_subscript_init(cubec_expression_subscript_t self,
                                 allocator_t allocator,
                                 cubec_expression_subscript_init_t *init) {
  if (!init)
    return;
  cubec_expression_init_t super_init = {
      .kind = CUBEC_NODE_EXPRESSION_SUBSCRIPT,
      .parent = NULL,
  };
  super_init.location = init->location;
  super_init.parent = init->parent;
  g_cubec_expression_type.init(&self->super, allocator, &super_init);
  self->host = init->host;
  self->index = init->index;
}

static void
_cubec_expression_subscript_dispose(cubec_expression_subscript_t self,
                                    allocator_t allocator) {
  allocator_free(allocator, &self->host);
  allocator_free(allocator, &self->index);
  g_cubec_expression_type.dispose(&self->super, allocator);
}

static void
_cubec_expression_subscript_clone(cubec_expression_subscript_t self,
                                  allocator_t allocator,
                                  cubec_expression_subscript_t another) {
  g_cubec_expression_type.clone(&self->super, allocator, &another->super);
  self->host = value_clone(allocator, another->host);
  self->index = value_clone(allocator, another->index);
  return;

cleanup:
  allocator_free(allocator, &self->index);
  allocator_free(allocator, &self->host);
}

static void
_cubec_expression_subscript_move(cubec_expression_subscript_t self,
                                 allocator_t allocator,
                                 cubec_expression_subscript_t another) {
  g_cubec_expression_type.move(&self->super, allocator, &another->super);
  self->host = value_move(allocator, another->host);
  self->index = value_move(allocator, another->index);
  return;

cleanup:
  allocator_free(allocator, &self->index);
  allocator_free(allocator, &self->host);
}

type_t g_cubec_expression_subscript_type = {
    .name = "cubec.cubec.expression_subscript",
    .size = sizeof(struct _cubec_expression_subscript_t),
    .init = (type_init_fn_t)_cubec_expression_subscript_init,
    .dispose = (type_dispose_fn_t)_cubec_expression_subscript_dispose,
    .clone = (type_clone_fn_t)_cubec_expression_subscript_clone,
    .move = (type_move_fn_t)_cubec_expression_subscript_move,
};

/* --------------------------------------------------------------------------
 *  Parser: read_expression_subscript
 * -------------------------------------------------------------------------- */

node_t read_expression_subscript(context_t ctx, vec_t tokens, size_t *position,
                                 const char *filename, node_t host) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;
  cubec_expression_subscript_t node = NULL;
  node_t index = NULL;
  token_t open_bracket = NULL;

  /* Expect '[' (caller ensures whitespace already skipped) */
  open_bracket = vec_get(tokens, current);
  if (!token_is(open_bracket, CUBEC_TOKEN_SYMBOL, "[")) {
    return NULL;
  }

  /* Look ahead to distinguish from slice: if we find ':' before ']', this is
   * a slice, not a subscript. */
  {
    size_t lookahead = current + 1;
    size_t depth = 0;
    bool found_colon = false;
    while (true) {
      token_t tok = vec_get(tokens, lookahead);
      if (!tok)
        break;
      if (token_is(tok, CUBEC_TOKEN_SYMBOL, "[")) {
        depth++;
      } else if (token_is(tok, CUBEC_TOKEN_SYMBOL, "]")) {
        if (depth == 0)
          break;
        depth--;
      } else if (token_is(tok, CUBEC_TOKEN_SYMBOL, ":")) {
        found_colon = true;
        break;
      }
      lookahead++;
    }
    if (found_colon) {
      /* This is a slice — let the slice parser handle it */
      return NULL;
    }
  }

  /* Commit to parsing a subscript */
  current++; /* consume '[' */

  skip_whitespace(tokens, &current);

  /* Parse index expression */
  index = read_expression(ctx, tokens, &current, filename);
  if (!index) {
    goto onerror;
  }

  skip_whitespace(tokens, &current);

  /* Expect ']' */
  token_t close_bracket = vec_get(tokens, current);
  if (!token_is(close_bracket, CUBEC_TOKEN_SYMBOL, "]")) {
    goto onerror;
  }
  current++;

  node = allocator_create(allocator, &g_cubec_expression_subscript_type,
                          &(cubec_expression_subscript_init_t){
                              .host = host,
                              .index = index,
                          });

  /* Location spans from '[' to ']' */
  {
    location_t loc = *token_get_location(open_bracket);
    loc.end = token_get_location(close_bracket)->end;
    loc.filename = filename;
    node->super.super.location = loc;
  }

  *position = current;
  return (node_t)node;

onerror:
  diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                       open_bracket ? *token_get_location(open_bracket)
                                    : (location_t){0},
                       "invalid subscript expression");
  ctx->error_count++;
  allocator_free(allocator, &index);
  /* host ownership: caller (read_value) owns it and will clean up */
  allocator_free(allocator, &node);
  return create_error(ctx, open_bracket ? *token_get_location(open_bracket)
                                        : (location_t){0});
}

/* --------------------------------------------------------------------------
 *  Factory: create_expression_subscript
 * -------------------------------------------------------------------------- */

node_t create_expression_subscript(context_t ctx, location_t loc, node_t host,
                                   node_t index) {
  allocator_t alloc = ctx->allocator;
  cubec_expression_subscript_init_t init = {
      .location = loc,
      .parent = NULL,
      .host = host,
      .index = index,
  };
  return (node_t)allocator_create(alloc, &g_cubec_expression_subscript_type,
                                  &init);
}

/* --------------------------------------------------------------------------
 *  Writer: write_expression_subscript
 * -------------------------------------------------------------------------- */

void write_expression_subscript(writer_t writer, node_t node) {
  cubec_expression_subscript_t sub = (cubec_expression_subscript_t)node;
  write_expression(writer, sub->host);
  writer_append(writer, "[");
  write_expression(writer, sub->index);
  writer_append(writer, "]");
}
