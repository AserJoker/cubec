#include "cubec/expression_call.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/token.h"
#include "cubec/expression.h"
#include "cubec/expression_spread.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_expression_call_init(cubec_expression_call_t self,
                                         allocator_t allocator,
                                         cubec_expression_call_init_t *init) {
  cubec_expression_init_t super_init = {
      .kind = CUBEC_NODE_EXPRESSION_CALL,
      .parent = NULL,
  };
  if (init) {
    super_init.location = init->location;
    super_init.parent = init->parent;
  }
  g_cubec_expression_type.init(&self->super, allocator, &super_init);

  self->callee = NULL;
  self->arguments = NULL;

  if (init) {
    self->callee = init->callee;
    if (init->arguments) {
      /* Take ownership of the caller's arguments vec directly */
      self->arguments = init->arguments;
    }
  }

  /* If no arguments vec was provided (e.g. clone path), create an empty one */
  if (!self->arguments) {
    self->arguments =
        allocator_create(allocator, &g_vec_type, &(vec_init_t){true});
  }
}

static void _cubec_expression_call_dispose(cubec_expression_call_t self,
                                            allocator_t allocator) {
  allocator_free(allocator, self->callee);
  self->callee = NULL;
  allocator_free(allocator, self->arguments);
  self->arguments = NULL;
  g_cubec_expression_type.dispose(&self->super, allocator);
}

static void _cubec_expression_call_clone(cubec_expression_call_t self,
                                          allocator_t allocator,
                                          cubec_expression_call_t another) {
  g_cubec_expression_type.clone(&self->super, allocator, &another->super);
  self->callee = value_clone(allocator, another->callee);

  size_t count = vec_get_size(another->arguments);
  for (size_t i = 0; i < count; i++) {
    node_t arg = (node_t)vec_get(another->arguments, i);
    vec_push(self->arguments, value_clone(allocator, arg));
  }
}

static void _cubec_expression_call_move(cubec_expression_call_t self,
                                         allocator_t allocator,
                                         cubec_expression_call_t another) {
  g_cubec_expression_type.move(&self->super, allocator, &another->super);
  self->callee = value_move(allocator, another->callee);

  /* Transfer arguments vec directly */
  allocator_free(allocator, self->arguments);
  self->arguments = another->arguments;
  another->arguments =
      allocator_create(allocator, &g_vec_type, &(vec_init_t){true});
}

type_t g_cubec_expression_call_type = {
    .name = "cubec.cubec.expression_call",
    .size = sizeof(struct _cubec_expression_call_t),
    .init = (type_init_fn_t)_cubec_expression_call_init,
    .dispose = (type_dispose_fn_t)_cubec_expression_call_dispose,
    .clone = (type_clone_fn_t)_cubec_expression_call_clone,
    .move = (type_move_fn_t)_cubec_expression_call_move,
};

/* --------------------------------------------------------------------------
 *  Parser: read_expression_call
 * -------------------------------------------------------------------------- */

node_t read_expression_call(allocator_t allocator, vec_t tokens,
                             size_t *position, const char *filename,
                             node_t callee) {
  size_t current = *position;
  cubec_expression_call_t node = NULL;
  vec_t arguments = NULL;

  /* Expect '(' (caller ensures whitespace already skipped).
   * Only return NULL without error if this is not a call at all. */
  token_t open_paren = vec_get(tokens, current);
  if (!token_is(open_paren, CUBEC_TOKEN_SYMBOL, "(")) {
    return NULL;
  }
  current++; /* Consumed '(' — committed to parsing a call from here */

  arguments =
      allocator_create(allocator, &g_vec_type, &(vec_init_t){true});

  /* Parse comma-separated arguments */
  bool expect_comma = false;
  while (true) {
    skip_whitespace(tokens, &current);

    token_t tok = vec_get(tokens, current);

    /* Check for closing ')' */
    if (token_is(tok, CUBEC_TOKEN_SYMBOL, ")")) {
      if (expect_comma) {
        goto onerror; /* trailing comma: f(a,) */
      }
      current++;
      break;
    }

    /* Parse one argument: try spread first, then regular expression */
    node_t arg =
        read_expression_spread(allocator, tokens, &current, filename);
    if (!arg) {
      arg =
          TRY_LOCAL(onerror, read_expression(allocator, tokens, &current, filename));
    }
    if (!arg) {
      goto onerror;
    }
    vec_push(arguments, arg);

    /* After an argument, expect either ',' or ')' */
    skip_whitespace(tokens, &current);
    tok = vec_get(tokens, current);
    if (token_is(tok, CUBEC_TOKEN_SYMBOL, ",")) {
      current++;
      expect_comma = true;
    } else if (token_is(tok, CUBEC_TOKEN_SYMBOL, ")")) {
      current++;
      break;
    } else {
      goto onerror;
    }
  }

  node = allocator_create(allocator, &g_cubec_expression_call_type,
                          &(cubec_expression_call_init_t){
                              .callee = callee,
                              .arguments = arguments,
                          });
  /* NOTE: arguments ownership has been transferred to node via init —
   *        do NOT free it here. */

  /* Location spans from '(' to ')' */
  {
    token_t close_paren = vec_get(tokens, current - 1);
    location_t loc = *token_get_location(open_paren);
    loc.end = token_get_location(close_paren)->end;
    node->super.super.location = loc;
    node->super.super.location.filename = filename;
  }

  *position = current;
  return (node_t)node;

onerror:
  /* autodispose=true: freeing arguments also frees all its elements */
  allocator_free(allocator, arguments);
  /* NOTE: callee is NOT freed here — the caller (read_value) still owns the
   *       pointer and will clean it up when the error propagates via TRY_LOCAL */
  allocator_free(allocator, node);
  {
    location_t *loc = token_get_location(open_paren);
    THROW(NULL, "%s:%" PRIuPTR ":%" PRIuPTR " invalid function call arguments",
          filename, loc->begin.line + 1, loc->begin.column + 1);
  }
}
