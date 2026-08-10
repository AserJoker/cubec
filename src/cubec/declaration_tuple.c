#include "cubec/declaration_tuple.h"
#include "core/emit_context.h"
#include "core/token.h"
#include "core/token_writer.h"
#include "cubec/expression_spread.h"
#include "cubec/expression_wildcard.h"
#include "cubec/node_error.h"
#include "cubec/token.h"
#include <string.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void
_cubec_declaration_tuple_init(cubec_declaration_tuple_t self,
                                  allocator_t allocator,
                                  cubec_declaration_tuple_init_t *init) {
  if (!init)
    return;
  cubec_expression_init_t super_init = {
      .kind = CUBEC_NODE_DECLARATION_TUPLE,
      .parent = NULL,
  };
  super_init.location = init->location;
  super_init.parent = init->parent;
  g_cubec_expression_type.init(&self->super, allocator, &super_init);
  self->element_types = init->element_types;
}

static void
_cubec_declaration_tuple_dispose(cubec_declaration_tuple_t self,
                                     allocator_t allocator) {
  allocator_free(allocator, &self->element_types);
  g_cubec_expression_type.dispose(&self->super, allocator);
}

static void
_cubec_declaration_tuple_clone(cubec_declaration_tuple_t self,
                                   allocator_t allocator,
                                   cubec_declaration_tuple_t another) {
  g_cubec_expression_type.clone(&self->super, allocator, &another->super);
  self->element_types = another->element_types
                            ? alloc_clone(allocator, another->element_types)
                            : NULL;
  return;
}

static void
_cubec_declaration_tuple_move(cubec_declaration_tuple_t self,
                                  allocator_t allocator,
                                  cubec_declaration_tuple_t another) {
  g_cubec_expression_type.move(&self->super, allocator, &another->super);
  self->element_types = another->element_types;
  another->element_types = NULL;
}

type_t g_cubec_declaration_tuple_type = {
    .size = sizeof(struct _cubec_declaration_tuple_t),
    .name = "cubec.declaration_tuple",
    .init = (type_init_fn_t)_cubec_declaration_tuple_init,
    .dispose = (type_dispose_fn_t)_cubec_declaration_tuple_dispose,
    .clone = (type_clone_fn_t)_cubec_declaration_tuple_clone,
    .move = (type_move_fn_t)_cubec_declaration_tuple_move,
};

/* --------------------------------------------------------------------------
 *  Helper: check symbol
 * -------------------------------------------------------------------------- */

static bool _is_symbol(vec_t tokens, size_t position, const char *symbol) {
  token_t token = vec_get(tokens, position);
  if (!token)
    return false;
  return token_is(token, CUBEC_TOKEN_SYMBOL, symbol);
}

/* --------------------------------------------------------------------------
 *  Parser: read_declaration_tuple
 * -------------------------------------------------------------------------- */

node_t read_declaration_tuple(context_t ctx, vec_t tokens, size_t *position,
                                  const char *filename) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;
  /* In a type context, '<' starts a tuple type expression.
     Special cases:
       - <> is an empty tuple
       - <?> means "tuple constraint" (T extends <?> = T must be a tuple)
       - <T> is a single-element tuple
     Since '<' in a type context always means tuple, we parse greedily.
  */
  size_t start = *position;

  if (!_is_symbol(tokens, start, "<"))
    return NULL;

  location_t start_location = *token_get_location(vec_get(tokens, start));
  start_location.filename = filename;

  current = start + 1;
  skip_whitespace(tokens, &current);

  /* Check for <> — empty tuple */
  if (_is_symbol(tokens, current, ">")) {
    current++;
    vec_init_t vi = {.auto_dispose = true};
    vec_t element_types = (vec_t)allocator_create(allocator, &g_vec_type, &vi);
    location_t loc = *token_get_location(vec_get(tokens, start));
    loc.filename = filename;
    cubec_declaration_tuple_init_t init = {
        .location = loc,
        .parent = NULL,
        .element_types = element_types,
    };
    cubec_declaration_tuple_t node =
        (cubec_declaration_tuple_t)allocator_create(
            allocator, &g_cubec_declaration_tuple_type, &init);
    *position = current;
    return (node_t)node;
  }

  /* Check for <?> — tuple constraint wildcard */
  if (_is_symbol(tokens, current, "?")) {
    size_t after_q = current + 1;
    skip_whitespace(tokens, &after_q);
    if (_is_symbol(tokens, after_q, ">")) {
      /* <?> — return a wildcard node representing "any tuple type" constraint
       */
      current = after_q + 1;
      cubec_expression_wildcard_t wnode =
          (cubec_expression_wildcard_t)allocator_create(
              allocator, &g_cubec_expression_wildcard_type, NULL);
      location_t loc = *token_get_location(vec_get(tokens, start));
      loc.filename = filename;
      wnode->super.super.location = loc;
      wnode->is_tuple = true;
      *position = current;
      return (node_t)wnode;
    }
  }

  /* Parse element type expressions */
  vec_init_t vi = {.auto_dispose = true};
  vec_t element_types = (vec_t)allocator_create(allocator, &g_vec_type, &vi);

  while (true) {
    skip_whitespace(tokens, &current);

    /* Check for spread: ...Args in <...Args> */
    node_t elem = read_expression_spread(ctx, tokens, &current, filename);
    if (!elem) {
      elem = read_type_expression_primary(ctx, tokens, &current, filename);
    }
    if (!elem)
      break;

    vec_push(element_types, elem);
    skip_whitespace(tokens, &current);

    if (!_is_symbol(tokens, current, ","))
      break;
    current++; /* skip ',' */
  }

  /* Expect closing '>' */
  if (!_is_symbol(tokens, current, ">")) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, start_location,
                         "missing '>' in tuple type expression");
    allocator_free(allocator, &element_types);
    return create_error(ctx, start_location);
  }
  current++; /* skip '>' */

  /* Create the node */
  location_t loc = *token_get_location(vec_get(tokens, start));
  loc.filename = filename;
  cubec_declaration_tuple_init_t init = {
      .location = loc,
      .parent = NULL,
      .element_types = element_types,
  };
  cubec_declaration_tuple_t node =
      (cubec_declaration_tuple_t)allocator_create(
          allocator, &g_cubec_declaration_tuple_type, &init);

  *position = current;
  return (node_t)node;
}

/* --------------------------------------------------------------------------
 *  Factory: create_declaration_tuple
 * -------------------------------------------------------------------------- */

node_t create_declaration_tuple(context_t ctx, location_t loc,
                                    vec_t element_types) {
  allocator_t alloc = ctx->allocator;
  cubec_declaration_tuple_init_t init = {
      .location = loc,
      .parent = NULL,
      .element_types = element_types,
  };
  return (node_t)allocator_create(alloc, &g_cubec_declaration_tuple_type,
                                  &init);
}

/* --------------------------------------------------------------------------
 *  Writer: write_declaration_tuple
 * -------------------------------------------------------------------------- */

void emit_declaration_tuple(emit_context_t ctx, node_t node) {
  cubec_declaration_tuple_t tuple = (cubec_declaration_tuple_t)node;
  recover_comments_to(ctx, node->location.begin.offset);
  emit_symbol(ctx, "<");
  for (size_t i = 0; i < vec_get_size(tuple->element_types); i++) {
    if (i != 0) {
      emit_symbol(ctx, ",");
      emit_space(ctx);
    }
    emit_expression(ctx, vec_get(tuple->element_types, i));
  }
  emit_symbol(ctx, ">");
}
