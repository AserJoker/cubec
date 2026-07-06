#include "cubec/expression_slice.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/token.h"
#include "cubec/expression.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_expression_slice_init(cubec_expression_slice_t self,
                                          allocator_t allocator,
                                          cubec_expression_slice_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  cubec_expression_init_t super_init = {
      .kind = CUBEC_NODE_EXPRESSION_SLICE,
      .parent = NULL,
  };
  super_init.location = init->location;
  super_init.parent = init->parent;
  TRY_VOID_LOCAL(onerror, g_cubec_expression_type.init(&self->super, allocator, &super_init));

  self->host = init->host;
  self->start = init->start;
  self->length = init->length;
onerror:
  return;
}

static void _cubec_expression_slice_dispose(cubec_expression_slice_t self,
                                             allocator_t allocator) {
  allocator_free(allocator, &self->host);
  allocator_free(allocator, &self->start);
  allocator_free(allocator, &self->length);
  g_cubec_expression_type.dispose(&self->super, allocator);
}

static void _cubec_expression_slice_clone(cubec_expression_slice_t self,
                                           allocator_t allocator,
                                           cubec_expression_slice_t another) {
  TRY_VOID_LOCAL(cleanup, g_cubec_expression_type.clone(&self->super, allocator, &another->super));
  self->host = TRY_LOCAL(cleanup, value_clone(allocator, another->host));
  self->start = TRY_LOCAL(cleanup, value_clone(allocator, another->start));
  self->length = TRY_LOCAL(cleanup, value_clone(allocator, another->length));
  return;

cleanup:
  allocator_free(allocator, &self->length);
  allocator_free(allocator, &self->start);
  allocator_free(allocator, &self->host);
}

static void _cubec_expression_slice_move(cubec_expression_slice_t self,
                                          allocator_t allocator,
                                          cubec_expression_slice_t another) {
  TRY_VOID_LOCAL(cleanup, g_cubec_expression_type.move(&self->super, allocator, &another->super));
  self->host = TRY_LOCAL(cleanup, value_move(allocator, another->host));
  self->start = TRY_LOCAL(cleanup, value_move(allocator, another->start));
  self->length = TRY_LOCAL(cleanup, value_move(allocator, another->length));
  return;

cleanup:
  allocator_free(allocator, &self->length);
  allocator_free(allocator, &self->start);
  allocator_free(allocator, &self->host);
}

type_t g_cubec_expression_slice_type = {
    .name = "cubec.cubec.expression_slice",
    .size = sizeof(struct _cubec_expression_slice_t),
    .init = (type_init_fn_t)_cubec_expression_slice_init,
    .dispose = (type_dispose_fn_t)_cubec_expression_slice_dispose,
    .clone = (type_clone_fn_t)_cubec_expression_slice_clone,
    .move = (type_move_fn_t)_cubec_expression_slice_move,
};

/* --------------------------------------------------------------------------
 *  Parser: read_expression_slice
 * -------------------------------------------------------------------------- */

node_t read_expression_slice(allocator_t allocator, vec_t tokens,
                              size_t *position, const char *filename,
                              node_t host) {
  size_t current = *position;
  cubec_expression_slice_t node = NULL;
  node_t start = NULL;
  node_t length = NULL;
  token_t open_bracket = NULL;
  token_t colon = NULL;
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

    /* Quick check: if next token is ':', it's definitely a slice (length-only) */
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
        if (!tok) break;

        /* Track bracket depth for potential generic args */
        if (token_is(tok, CUBEC_TOKEN_SYMBOL, "[")) {
          expr_depth++;
        } else if (token_is(tok, CUBEC_TOKEN_SYMBOL, "]")) {
          if (expr_depth == 0) break; /* End of this bracket group */
          expr_depth--;
        } else if (token_is(tok, CUBEC_TOKEN_SYMBOL, ":")) {
          found_colon = true;
          break;
        } else if (token_is(tok, CUBEC_TOKEN_SYMBOL, ",")) {
          if (expr_depth == 0) break; /* Comma in generic args */
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
    colon = tok;
    current++; /* Consumed ':' */
    skip_whitespace(tokens, &current);

    /* Parse optional length expression */
    peek = vec_get(tokens, current);
    if (!token_is(peek, CUBEC_TOKEN_SYMBOL, "]")) {
      length = TRY_LOCAL(onerror,
                         read_expression(allocator, tokens, &current, filename));
      if (!length) {
        goto onerror;
      }
    }
  } else {
    /* Parse start expression first */
    start = TRY_LOCAL(onerror,
                      read_expression(allocator, tokens, &current, filename));
    if (!start) {
      goto onerror;
    }

    skip_whitespace(tokens, &current);

    /* Expect ':' after start */
    tok = vec_get(tokens, current);
    if (!token_is(tok, CUBEC_TOKEN_SYMBOL, ":")) {
      goto onerror;
    }
    colon = tok;
    current++; /* Consumed ':' */
    skip_whitespace(tokens, &current);

    /* Parse optional length expression */
    peek = vec_get(tokens, current);
    if (!token_is(peek, CUBEC_TOKEN_SYMBOL, "]")) {
      length = TRY_LOCAL(onerror,
                         read_expression(allocator, tokens, &current, filename));
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

  node = TRY_LOCAL(onerror, allocator_create(allocator, &g_cubec_expression_slice_type,
                          &(cubec_expression_slice_init_t){
                              .host = host,
                              .start = start,
                              .length = length,
                          }));

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
  allocator_free(allocator, &start);
  allocator_free(allocator, &length);
  /* host ownership: caller (read_value) owns it and will clean up */
  allocator_free(allocator, &node);
  {
    location_t *loc = token_get_location(open_bracket);
    THROW(NULL, "%s:%" PRIuPTR ":%" PRIuPTR " invalid slice expression",
          filename, loc->begin.line + 1, loc->begin.column + 1);
  }
}