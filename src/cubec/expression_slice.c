#include "core/emit_context.h"
#include "core/token_writer.h"
#include "cubec/expression_slice.h"
#include "core/token.h"
#include "cubec/expression.h"
#include "cubec/node_error.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_expression_slice_init(cubec_expression_slice_t self,
                                         allocator_t allocator,
                                         cubec_expression_slice_init_t *init) {
  if (!init)
    return;
  cubec_expression_init_t super_init = {
      .kind = CUBEC_NODE_EXPRESSION_SLICE,
      .parent = NULL,
  };
  super_init.location = init->location;
  super_init.parent = init->parent;
  g_cubec_expression_class.init(&self->super, allocator, &super_init);

  self->host = init->host;
  self->start = init->start;
  self->length = init->length;
}

static void _cubec_expression_slice_dispose(cubec_expression_slice_t self,
                                            allocator_t allocator) {
  allocator_free(allocator, &self->host);
  allocator_free(allocator, &self->start);
  allocator_free(allocator, &self->length);
  g_cubec_expression_class.dispose(&self->super, allocator);
}

static void _cubec_expression_slice_clone(cubec_expression_slice_t self,
                                          allocator_t allocator,
                                          cubec_expression_slice_t another) {
  g_cubec_expression_class.clone(&self->super, allocator, &another->super);
  self->host = alloc_clone(allocator, another->host);
  self->start = alloc_clone(allocator, another->start);
  self->length = alloc_clone(allocator, another->length);
  return;

cleanup:
  allocator_free(allocator, &self->length);
  allocator_free(allocator, &self->start);
  allocator_free(allocator, &self->host);
}

static void _cubec_expression_slice_move(cubec_expression_slice_t self,
                                         allocator_t allocator,
                                         cubec_expression_slice_t another) {
  g_cubec_expression_class.move(&self->super, allocator, &another->super);
  self->host = alloc_move(allocator, another->host);
  self->start = alloc_move(allocator, another->start);
  self->length = alloc_move(allocator, another->length);
  return;

cleanup:
  allocator_free(allocator, &self->length);
  allocator_free(allocator, &self->start);
  allocator_free(allocator, &self->host);
}

class_t g_cubec_expression_slice_class = {
    .name = "cubec.cubec.expression_slice",
    .size = sizeof(struct _cubec_expression_slice_t),
    .init = (class_init_fn_t)_cubec_expression_slice_init,
    .dispose = (class_dispose_fn_t)_cubec_expression_slice_dispose,
    .clone = (class_clone_fn_t)_cubec_expression_slice_clone,
    .move = (class_move_fn_t)_cubec_expression_slice_move,
};

/* --------------------------------------------------------------------------
 *  Parser: read_expression_slice
 * -------------------------------------------------------------------------- */

node_t read_expression_slice(context_t ctx, vec_t tokens, size_t *position,
                             const char *filename, node_t host) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;
  cubec_expression_slice_t node = NULL;
  node_t start = NULL;
  node_t length = NULL;
  token_t open_bracket = NULL;
  token_t peek = NULL;

  /* Expect '[' (caller ensures whitespace already skipped).
   * Only return NULL without error if this is not a slice at all. */
  open_bracket = vec_get(tokens, current);
  if (!token_is(open_bracket, CUBEC_TOKEN_SYMBOL, "[")) {
    return NULL;
  }

  /* Look ahead before committing: we need to confirm this is a slice.
   * Parse past an optional expression to find ':'. If no ':' is found
   * before ']' or ',', this is not a slice — caller will try generic. */
  {
    size_t lookahead = current + 1; /* skip '[' itself */
    skip_whitespace(tokens, &lookahead);

    /* Quick check: if next token is ':', it's definitely a slice (length-only)
     */
    token_t tok = vec_get(tokens, lookahead);
    if (token_is(tok, CUBEC_TOKEN_SYMBOL, ":")) {
      /* Confirmed: this is a slice starting with [:length] */
      /* Continue to full parsing below */
    } else {
      /* Next token is not ':', so try parsing an expression first.
       * We need to simulate read_expression to find where it ends. */
      /* Skip tokens until we find ':' or ']' or ',' */
      /* This handles cases like arr[0:10] vs arr[0] */
      bool found_colon = false;
      size_t expr_depth = 0;
      while (true) {
        tok = vec_get(tokens, lookahead);
        if (!tok)
          break;

        /* Track bracket depth for potential generic args */
        if (token_is(tok, CUBEC_TOKEN_SYMBOL, "[")) {
          expr_depth++;
        } else if (token_is(tok, CUBEC_TOKEN_SYMBOL, "]")) {
          if (expr_depth == 0)
            break; /* End of this bracket group */
          expr_depth--;
        } else if (token_is(tok, CUBEC_TOKEN_SYMBOL, ":")) {
          found_colon = true;
          break;
        } else if (token_is(tok, CUBEC_TOKEN_SYMBOL, ",")) {
          if (expr_depth == 0)
            break; /* Comma in generic args */
        }
        lookahead++;
      }

      if (!found_colon) {
        /* No ':' found — this is not a slice, caller will try generic */
        return NULL;
      }
    }
  }

  /* We have confirmed this is a slice. Commit to parsing. */
  current++; /* Consumed '[' — now committed to parsing a slice from here */

  skip_whitespace(tokens, &current);

  /* Look for ':' to determine what we're parsing */
  token_t tok = vec_get(tokens, current);
  if (token_is(tok, CUBEC_TOKEN_SYMBOL, ":")) {
    /* Slice starts with ':' — only length will be specified */
    current++; /* Consumed ':' */
    skip_whitespace(tokens, &current);

    /* Parse optional length expression */
    peek = vec_get(tokens, current);
    if (!token_is(peek, CUBEC_TOKEN_SYMBOL, "]")) {
      length = read_expression(ctx, tokens, &current, filename);
      if (!length) {
        goto onerror;
      }
    }
  } else {
    /* Parse start expression first */
    start = read_expression(ctx, tokens, &current, filename);
    if (!start) {
      goto onerror;
    }

    skip_whitespace(tokens, &current);

    /* Expect ':' after start */
    tok = vec_get(tokens, current);
    if (!token_is(tok, CUBEC_TOKEN_SYMBOL, ":")) {
      goto onerror;
    }
    current++; /* Consumed ':' */
    skip_whitespace(tokens, &current);

    /* Parse optional length expression */
    peek = vec_get(tokens, current);
    if (!token_is(peek, CUBEC_TOKEN_SYMBOL, "]")) {
      length = read_expression(ctx, tokens, &current, filename);
      if (!length) {
        goto onerror;
      }
    }
  }

  /* Expect ']' */
  skip_whitespace(tokens, &current);
  tok = vec_get(tokens, current);
  if (!token_is(tok, CUBEC_TOKEN_SYMBOL, "]")) {
    goto onerror;
  }
  current++;

  node = allocator_create(allocator, &g_cubec_expression_slice_class,
                          &(cubec_expression_slice_init_t){
                              .host = host,
                              .start = start,
                              .length = length,
                          });

  /* Location spans from '[' to ']' */
  {
    token_t close_bracket = vec_get(tokens, current - 1);
    location_t loc = *token_get_location(open_bracket);
    loc.end = token_get_location(close_bracket)->end;
    node->super.super.location = loc;
    node->super.super.location.filename = filename;
  }

  *position = current;
  return (node_t)node;

onerror:
  diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                       open_bracket ? *token_get_location(open_bracket)
                                    : (location_t){0},
                       "invalid slice expression syntax");
  allocator_free(allocator, &start);
  allocator_free(allocator, &length);
  /* host ownership: caller (read_value) owns it and will clean up */
  allocator_free(allocator, &node);
  return create_error(ctx, open_bracket ? *token_get_location(open_bracket)
                                        : (location_t){0});
}

/* --------------------------------------------------------------------------
 *  Factory: create_expression_slice
 * -------------------------------------------------------------------------- */

node_t create_expression_slice(context_t ctx, location_t loc, node_t host,
                               node_t start, node_t length) {
  allocator_t alloc = ctx->allocator;
  cubec_expression_slice_init_t init = {
      .host = host, .start = start, .length = length};
  return (node_t)allocator_create(alloc, &g_cubec_expression_slice_class, &init);
}

/* --------------------------------------------------------------------------
 *  Writer: write_expression_slice
 * -------------------------------------------------------------------------- */

void emit_expression_slice(emit_context_t ctx, node_t node) {
  cubec_expression_slice_t slice = (cubec_expression_slice_t)node;
  recover_comments_to(ctx, node->location.begin.offset);
  emit_expression(ctx, slice->host);
  emit_symbol(ctx, "[");
  if (slice->start) emit_expression(ctx, slice->start);
  emit_symbol(ctx, ":");
  if (slice->length) emit_expression(ctx, slice->length);
  emit_symbol(ctx, "]");
}