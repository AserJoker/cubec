#include "core/emit_context.h"
#include "core/token_writer.h"
#include "cubec/expression_subscript.h"
#include "core/token.h"
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
  g_cubec_expression_class.init(&self->super, allocator, &super_init);
  self->host = init->host;
  self->index = init->index;
}

static void
_cubec_expression_subscript_dispose(cubec_expression_subscript_t self,
                                    allocator_t allocator) {
  allocator_free(allocator, &self->host);
  allocator_free(allocator, &self->index);
  g_cubec_expression_class.dispose(&self->super, allocator);
}

static void
_cubec_expression_subscript_clone(cubec_expression_subscript_t self,
                                  allocator_t allocator,
                                  cubec_expression_subscript_t another) {
  g_cubec_expression_class.clone(&self->super, allocator, &another->super);
  self->host = alloc_clone(allocator, another->host);
  self->index = alloc_clone(allocator, another->index);
  return;

cleanup:
  allocator_free(allocator, &self->index);
  allocator_free(allocator, &self->host);
}

static void
_cubec_expression_subscript_move(cubec_expression_subscript_t self,
                                 allocator_t allocator,
                                 cubec_expression_subscript_t another) {
  g_cubec_expression_class.move(&self->super, allocator, &another->super);
  self->host = alloc_move(allocator, another->host);
  self->index = alloc_move(allocator, another->index);
  return;

cleanup:
  allocator_free(allocator, &self->index);
  allocator_free(allocator, &self->host);
}

class_t g_cubec_expression_subscript_class = {
    .name = "cubec.cubec.expression_subscript",
    .size = sizeof(struct _cubec_expression_subscript_t),
    .init = (class_init_fn_t)_cubec_expression_subscript_init,
    .dispose = (class_dispose_fn_t)_cubec_expression_subscript_dispose,
    .clone = (class_clone_fn_t)_cubec_expression_subscript_clone,
    .move = (class_move_fn_t)_cubec_expression_subscript_move,
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

  node = allocator_create(allocator, &g_cubec_expression_subscript_class,
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
  return (node_t)allocator_create(alloc, &g_cubec_expression_subscript_class,
                                  &init);
}

/* --------------------------------------------------------------------------
 *  Writer: write_expression_subscript
 * -------------------------------------------------------------------------- */

void emit_expression_subscript(emit_context_t ctx, node_t node) {
  cubec_expression_subscript_t sub = (cubec_expression_subscript_t)node;
  recover_comments_to(ctx, node->location.begin.offset);
  emit_expression(ctx, sub->host);
  emit_symbol(ctx, "[");
  emit_expression(ctx, sub->index);
  emit_symbol(ctx, "]");
}
