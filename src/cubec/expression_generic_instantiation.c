#include "cubec/expression_generic_instantiation.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/token.h"
#include "cubec/ast_factory.h"
#include "cubec/ast_factory_internal.h"
#include "cubec/expression.h"
#include "cubec/expression_spread.h"
#include "cubec/literal_identifier.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_expression_generic_instantiation_init(
    cubec_expression_generic_instantiation_t self, allocator_t allocator,
    cubec_expression_generic_instantiation_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  cubec_expression_init_t super_init = {
      .kind = CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION,
      .parent = NULL,
  };
  super_init.location = init->location;
  super_init.parent = init->parent;
  TRY_VOID_LOCAL(onerror, g_cubec_expression_type.init(&self->super, allocator, &super_init));

  self->callee = init->callee;
  if (init->arguments) {
    /* Take ownership of the caller's arguments vec directly */
    self->arguments = init->arguments;
  } else {
    /* If no arguments vec was provided (e.g. clone path), create an empty one */
    self->arguments =
        TRY_LOCAL(onerror, allocator_create(allocator, &g_vec_type, &(vec_init_t){true}));
  }
onerror:
  return;
}

static void _cubec_expression_generic_instantiation_dispose(
    cubec_expression_generic_instantiation_t self, allocator_t allocator) {
  allocator_free(allocator, &self->callee);
  allocator_free(allocator, &self->arguments);
  g_cubec_expression_type.dispose(&self->super, allocator);
}

static void _cubec_expression_generic_instantiation_clone(
    cubec_expression_generic_instantiation_t self, allocator_t allocator,
    cubec_expression_generic_instantiation_t another) {
  TRY_VOID_LOCAL(cleanup, g_cubec_expression_type.clone(&self->super, allocator, &another->super));
  self->callee = TRY_LOCAL(cleanup, value_clone(allocator, another->callee));
  self->arguments = TRY_LOCAL(cleanup, value_clone(allocator, another->arguments));
  return;

cleanup:
  allocator_free(allocator, &self->callee);
  allocator_free(allocator, &self->arguments);
}

static void _cubec_expression_generic_instantiation_move(
    cubec_expression_generic_instantiation_t self, allocator_t allocator,
    cubec_expression_generic_instantiation_t another) {
  TRY_VOID_LOCAL(cleanup, g_cubec_expression_type.move(&self->super, allocator, &another->super));
  self->callee = TRY_LOCAL(cleanup, value_move(allocator, another->callee));

  /* Transfer arguments vec directly */
  allocator_free(allocator, &self->arguments);
  self->arguments = another->arguments;
  another->arguments =
      TRY_LOCAL(cleanup, allocator_create(allocator, &g_vec_type, &(vec_init_t){true}));
  return;

cleanup:
  allocator_free(allocator, &self->callee);
  /* self->arguments is in inconsistent state, but since callee failed,
   * the move is aborted and original another->arguments is still valid */
}

type_t g_cubec_expression_generic_instantiation_type = {
    .name = "cubec.cubec.expression_generic_instantiation",
    .size = sizeof(struct _cubec_expression_generic_instantiation_t),
    .init =
        (type_init_fn_t)_cubec_expression_generic_instantiation_init,
    .dispose =
        (type_dispose_fn_t)_cubec_expression_generic_instantiation_dispose,
    .clone =
        (type_clone_fn_t)_cubec_expression_generic_instantiation_clone,
    .move =
        (type_move_fn_t)_cubec_expression_generic_instantiation_move,
};

/* --------------------------------------------------------------------------
 *  Parser: read_expression_generic_instantiation
 * -------------------------------------------------------------------------- */

node_t read_expression_generic_instantiation(allocator_t allocator,
                                             vec_t tokens, size_t *position,
                                             const char *filename,
                                             node_t callee) {
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

  arguments =
      TRY_LOCAL(onerror, allocator_create(allocator, &g_vec_type, &(vec_init_t){true}));

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
    node_t arg =
        read_expression_spread(allocator, tokens, &current, filename);
    if (!arg) {
      arg = TRY_LOCAL(onerror,
                      read_expression_base(allocator, tokens, &current, filename));
    }
    if (!arg) {
      /* Try wildcard '?' as a generic argument */
      skip_whitespace(tokens, &current);
      token_t question_tok = vec_get(tokens, current);
      if (token_is(question_tok, CUBEC_TOKEN_SYMBOL, "?")) {
        /* Create a wildcard placeholder node */
        arg = TRY_LOCAL(onerror, allocator_create(
            allocator, &g_cubec_literal_identifier_type,
            &(cubec_literal_identifier_init_t){
                .location = *token_get_location(question_tok),
                .parent = NULL,
                .value = "?",
            }));
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

  node = TRY_LOCAL(onerror, allocator_create(
      allocator, &g_cubec_expression_generic_instantiation_type,
      &(cubec_expression_generic_instantiation_init_t){
          .callee = callee,
          .arguments = arguments,
      }));
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
  /* autodispose=true: freeing arguments also frees all its elements */
  allocator_free(allocator, &arguments);
  /* NOTE: callee is NOT freed here — the caller (read_value) still owns the
   *       pointer and will clean it up when the error propagates via TRY_LOCAL */
  allocator_free(allocator, &node);
  {
    location_t *loc = token_get_location(open_bracket);
    THROW(NULL,
          "%s:%" PRIuPTR ":%" PRIuPTR
          " invalid generic instantiation arguments",
          filename, loc->begin.line + 1, loc->begin.column + 1);
  }
}

/* --------------------------------------------------------------------------
 *  Factory: cubec_ast_create_generic_instantiation
 * -------------------------------------------------------------------------- */

node_t cubec_ast_create_generic_instantiation(allocator_t alloc,
                                              location_t loc,
                                              node_t callee, vec_t args) {
  cubec_expression_generic_instantiation_init_t init = {
      .location = loc, .parent = NULL, .callee = callee, .arguments = args};
  return (node_t)allocator_create(
      alloc, &g_cubec_expression_generic_instantiation_type, &init);
}
