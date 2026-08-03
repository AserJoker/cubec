#include "cubec/expression_call.h"
#include "core/token.h"
#include "core/vec.h"
#include "core/writer.h"
#include "cubec/expression.h"
#include "cubec/expression_spread.h"
#include "cubec/node_error.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_expression_call_init(cubec_expression_call_t self,
                                        allocator_t allocator,
                                        cubec_expression_call_init_t *init) {
  if (!init)
    return;
  cubec_expression_init_t super_init = {
      .kind = CUBEC_NODE_EXPRESSION_CALL,
      .parent = NULL,
  };
  super_init.location = init->location;
  super_init.parent = init->parent;
  g_cubec_expression_type.init(&self->super, allocator, &super_init);

  self->callee = init->callee;
  if (init->arguments) {
    /* Take ownership of the caller's arguments vec directly */
    self->arguments = init->arguments;
  } else {
    /* If no arguments vec was provided (e.g. clone path), create an empty one
     */
    self->arguments =
        allocator_create(allocator, &g_vec_type, &(vec_init_t){true});
  }
}

static void _cubec_expression_call_dispose(cubec_expression_call_t self,
                                           allocator_t allocator) {
  allocator_free(allocator, &self->callee);
  allocator_free(allocator, &self->arguments);
  g_cubec_expression_type.dispose(&self->super, allocator);
}

static void _cubec_expression_call_clone(cubec_expression_call_t self,
                                         allocator_t allocator,
                                         cubec_expression_call_t another) {
  g_cubec_expression_type.clone(&self->super, allocator, &another->super);
  self->callee = value_clone(allocator, another->callee);
  self->arguments = value_clone(allocator, another->arguments);
  return;

cleanup:
  allocator_free(allocator, &self->callee);
  allocator_free(allocator, &self->arguments);
}

static void _cubec_expression_call_move(cubec_expression_call_t self,
                                        allocator_t allocator,
                                        cubec_expression_call_t another) {
  g_cubec_expression_type.move(&self->super, allocator, &another->super);
  self->callee = value_move(allocator, another->callee);

  /* Transfer arguments vec directly */
  allocator_free(allocator, &self->arguments);
  self->arguments = another->arguments;
  another->arguments =
      allocator_create(allocator, &g_vec_type, &(vec_init_t){true});
  return;

cleanup:
  allocator_free(allocator, &self->callee);
  /* self->arguments may be partially set, but since move failed, the callee
   * already failed, so we just need to ensure consistency */
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

node_t read_expression_call(context_t ctx, vec_t tokens, size_t *position,
                            const char *filename, node_t callee) {
  allocator_t allocator = ctx->allocator;
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

  arguments = allocator_create(allocator, &g_vec_type, &(vec_init_t){true});

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
    node_t arg = read_expression_spread(ctx, tokens, &current, filename);
    if (!arg) {
      arg = read_expression_base(ctx, tokens, &current, filename);
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
  diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                       open_paren ? *token_get_location(open_paren)
                                  : (location_t){0},
                       "invalid function call syntax");
  /* autodispose=true: freeing arguments also frees all its elements */
  allocator_free(allocator, &arguments);
  /* NOTE: callee is NOT freed here — the caller (read_value) still owns the
   *       pointer and will clean it up when the error propagates */
  allocator_free(allocator, &node);
  return create_error(ctx, open_paren ? *token_get_location(open_paren)
                                      : (location_t){0});
}

/* --------------------------------------------------------------------------
 *  Factory: create_expression_call
 * -------------------------------------------------------------------------- */

node_t create_expression_call(context_t ctx, location_t loc, node_t callee,
                              vec_t args) {
  allocator_t alloc = ctx->allocator;
  cubec_expression_call_init_t init = {.callee = callee, .arguments = args};
  return (node_t)allocator_create(alloc, &g_cubec_expression_call_type, &init);
}

void write_expression_call(writer_t writer, node_t node) {
  cubec_expression_call_t expr = (cubec_expression_call_t)node;
  write_expression(writer, expr->callee);
  writer_append(writer, "(");
  for (size_t idx = 0; idx < vec_get_size(expr->arguments); idx++) {
    if (idx != 0) {
      writer_append(writer, ", ");
    }
    node_t arg = vec_get(expr->arguments, idx);
    write_expression(writer, arg);
  }
  writer_append(writer, ")");
}
