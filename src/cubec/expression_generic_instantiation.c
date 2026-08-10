#include "cubec/expression_generic_instantiation.h"
#include "core/emit_context.h"
#include "core/token.h"
#include "core/token_writer.h"
#include "cubec/expression.h"
#include "cubec/expression_spread.h"
#include "cubec/literal_identifier.h"
#include "cubec/node_error.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_expression_generic_instantiation_init(
    cubec_expression_generic_instantiation_t self, allocator_t allocator,
    cubec_expression_generic_instantiation_init_t *init) {
  if (!init)
    return;
  cubec_expression_init_t super_init = {
      .kind = CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION,
      .parent = NULL,
  };
  super_init.location = init->location;
  super_init.parent = init->parent;
  g_cubec_expression_class.init(&self->super, allocator, &super_init);

  self->callee = init->callee;
  if (init->arguments) {
    /* Take ownership of the caller's arguments vec directly */
    self->arguments = init->arguments;
  } else {
    /* If no arguments vec was provided (e.g. clone path), create an empty one
     */
    self->arguments =
        allocator_create(allocator, &g_vec_class, &(vec_init_t){true});
  }
}

static void _cubec_expression_generic_instantiation_dispose(
    cubec_expression_generic_instantiation_t self, allocator_t allocator) {
  allocator_free(allocator, &self->callee);
  allocator_free(allocator, &self->arguments);
  g_cubec_expression_class.dispose(&self->super, allocator);
}

static void _cubec_expression_generic_instantiation_clone(
    cubec_expression_generic_instantiation_t self, allocator_t allocator,
    cubec_expression_generic_instantiation_t another) {
  g_cubec_expression_class.clone(&self->super, allocator, &another->super);
  self->callee = alloc_clone(allocator, another->callee);
  self->arguments = alloc_clone(allocator, another->arguments);
  return;

cleanup:
  allocator_free(allocator, &self->callee);
  allocator_free(allocator, &self->arguments);
}

static void _cubec_expression_generic_instantiation_move(
    cubec_expression_generic_instantiation_t self, allocator_t allocator,
    cubec_expression_generic_instantiation_t another) {
  g_cubec_expression_class.move(&self->super, allocator, &another->super);
  self->callee = alloc_move(allocator, another->callee);

  /* Transfer arguments vec directly */
  allocator_free(allocator, &self->arguments);
  self->arguments = another->arguments;
  another->arguments =
      allocator_create(allocator, &g_vec_class, &(vec_init_t){true});
  return;

cleanup:
  allocator_free(allocator, &self->callee);
  /* self->arguments is in inconsistent state, but since callee failed,
   * the move is aborted and original another->arguments is still valid */
}

class_t g_cubec_expression_generic_instantiation_class = {
    .name = "cubec.cubec.expression_generic_instantiation",
    .size = sizeof(struct _cubec_expression_generic_instantiation_t),
    .init = (class_init_fn_t)_cubec_expression_generic_instantiation_init,
    .dispose =
        (class_dispose_fn_t)_cubec_expression_generic_instantiation_dispose,
    .clone = (class_clone_fn_t)_cubec_expression_generic_instantiation_clone,
    .move = (class_move_fn_t)_cubec_expression_generic_instantiation_move,
};

/* --------------------------------------------------------------------------
 *  Parser: read_expression_generic_instantiation
 * -------------------------------------------------------------------------- */

node_t read_expression_generic_instantiation(context_t ctx, vec_t tokens,
                                             size_t *position,
                                             const char *filename,
                                             node_t callee) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;
  cubec_expression_generic_instantiation_t node = NULL;
  vec_t arguments = NULL;

  /* Expect '[' (caller ensures whitespace already skipped).
   * Only return NULL without error if this is not a generic instantiation. */
  token_t open_bracket = vec_get(tokens, current);
  if (!token_is(open_bracket, CUBEC_TOKEN_SYMBOL, "[")) {
    return NULL;
  }
  current++; /* Consumed '[' — committed to parsing from here */

  arguments = allocator_create(allocator, &g_vec_class, &(vec_init_t){true});

  /* Parse comma-separated arguments */
  bool expect_comma = false;
  while (true) {
    skip_whitespace(tokens, &current);

    token_t tok = vec_get(tokens, current);

    /* Check for closing ']' */
    if (token_is(tok, CUBEC_TOKEN_SYMBOL, "]")) {
      if (expect_comma) {
        goto onerror; /* trailing comma: f[a,] */
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
      /* Try wildcard '?' as a generic argument */
      skip_whitespace(tokens, &current);
      token_t question_tok = vec_get(tokens, current);
      if (token_is(question_tok, CUBEC_TOKEN_SYMBOL, "?")) {
        /* Create a wildcard placeholder node */
        arg =
            allocator_create(allocator, &g_cubec_literal_identifier_class,
                             &(cubec_literal_identifier_init_t){
                                 .location = *token_get_location(question_tok),
                                 .parent = NULL,
                                 .value = "?",
                             });
        current++;
      } else {
        goto onerror;
      }
    }
    vec_push(arguments, arg);

    /* After an argument, expect either ',' or ']' */
    skip_whitespace(tokens, &current);
    tok = vec_get(tokens, current);
    if (token_is(tok, CUBEC_TOKEN_SYMBOL, ",")) {
      current++;
      expect_comma = true;
    } else if (token_is(tok, CUBEC_TOKEN_SYMBOL, "]")) {
      current++;
      break;
    } else {
      goto onerror;
    }
  }

  node = allocator_create(allocator,
                          &g_cubec_expression_generic_instantiation_class,
                          &(cubec_expression_generic_instantiation_init_t){
                              .callee = callee,
                              .arguments = arguments,
                          });
  /* NOTE: arguments ownership has been transferred to node via init —
   *        do NOT free it here. */

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
                       "invalid generic instantiation syntax");
  /* autodispose=true: freeing arguments also frees all its elements */
  allocator_free(allocator, &arguments);
  /* NOTE: callee is NOT freed here — the caller (read_value) still owns the
   *       pointer and will clean it up when the error propagates */
  allocator_free(allocator, &node);
  return create_error(ctx, open_bracket ? *token_get_location(open_bracket)
                                        : (location_t){0});
}

/* --------------------------------------------------------------------------
 *  Factory: create_expression_generic_instantiation
 * -------------------------------------------------------------------------- */

node_t create_expression_generic_instantiation(context_t ctx, location_t loc,
                                               node_t callee, vec_t args) {
  allocator_t alloc = ctx->allocator;
  cubec_expression_generic_instantiation_init_t init = {
      .location = loc, .parent = NULL, .callee = callee, .arguments = args};
  return (node_t)allocator_create(
      alloc, &g_cubec_expression_generic_instantiation_class, &init);
}

/* --------------------------------------------------------------------------
 *  Writer: write_expression_generic_instantiation
 * -------------------------------------------------------------------------- */

void emit_expression_generic_instantiation(emit_context_t ctx, node_t node) {
  cubec_expression_generic_instantiation_t expr = (cubec_expression_generic_instantiation_t)node;
  recover_comments_to(ctx, node->location.begin.offset);
  emit_expression(ctx, expr->callee);
  emit_symbol(ctx, "[");
  for (size_t i = 0; i < vec_get_size(expr->arguments); i++) {
    if (i != 0) {
      emit_symbol(ctx, ",");
      emit_space(ctx);
    }
    emit_expression(ctx, vec_get(expr->arguments, i));
  }
  emit_symbol(ctx, "]");
}
