#include "cubec/declaration_array.h"
#include "cubec/ast_factory_internal.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/location.h"
#include "core/token.h"
#include "cubec/ast_factory.h"
#include "cubec/expression.h"
#include "cubec/literal_numeric.h"
#include "cubec/node.h"
#include "cubec/token.h"

static void _cubec_declaration_array_init(cubec_declaration_array_t self,
                                          allocator_t allocator,
                                          cubec_declaration_array_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  cubec_declaration_init_t super_init = {
      .kind = CUBEC_NODE_DECLARATION_ARRAY,
      .parent = NULL,
  };
  super_init.location = init->location;
  TRY_VOID_LOCAL(onerror, g_cubec_declaration_type.init(&self->super, allocator, &super_init));
  self->size = init->size;
  self->type = init->type;
onerror:
  return;
}

static void _cubec_declaration_array_dispose(cubec_declaration_array_t self,
                                             allocator_t allocator) {
  allocator_free(allocator, &self->size);
  allocator_free(allocator, &self->type);
  g_cubec_declaration_type.dispose(&self->super, allocator);
}

static void _cubec_declaration_array_clone(cubec_declaration_array_t self,
                                           allocator_t allocator,
                                           cubec_declaration_array_t another) {
  TRY_VOID_LOCAL(cleanup, g_cubec_declaration_type.clone(&self->super, allocator, &another->super));
  self->size = TRY_LOCAL(cleanup, value_clone(allocator, another->size));
  self->type = TRY_LOCAL(cleanup, value_clone(allocator, another->type));
  return;

cleanup:
  allocator_free(allocator, &self->size);
  allocator_free(allocator, &self->type);
}

static void _cubec_declaration_array_move(cubec_declaration_array_t self,
                                          allocator_t allocator,
                                          cubec_declaration_array_t another) {
  TRY_VOID_LOCAL(cleanup, g_cubec_declaration_type.move(&self->super, allocator, &another->super));
  self->size = TRY_LOCAL(cleanup, value_move(allocator, another->size));
  self->type = TRY_LOCAL(cleanup, value_move(allocator, another->type));
  return;

cleanup:
  allocator_free(allocator, &self->size);
  allocator_free(allocator, &self->type);
}

type_t g_cubec_declaration_array_type = {
    .name = "cubec.cubec.declaration_array",
    .size = sizeof(struct _cubec_declaration_array_t),
    .init = (type_init_fn_t)_cubec_declaration_array_init,
    .dispose = (type_dispose_fn_t)_cubec_declaration_array_dispose,
    .clone = (type_clone_fn_t)_cubec_declaration_array_clone,
    .move = (type_move_fn_t)_cubec_declaration_array_move,
};

node_t read_declaration_array(allocator_t allocator, vec_t tokens,
                              size_t *position, const char *filename) {
  size_t current = *position;
  cubec_declaration_array_t node = NULL;
  node_t size = NULL;
  node_t type = NULL;
  location_t start_location = {0};

  /* Expect '[' (array indicator) */
  token_t open_bracket = TRY_LOCAL(onerror, vec_get(tokens, current));
  if (!token_is(open_bracket, CUBEC_TOKEN_SYMBOL, "[")) {
    return NULL;
  }
  current++;

  /* Skip whitespace/comments before checking for slice or parsing expression */
  skip_whitespace(tokens, &current);

  /* Check if this is a slice (empty brackets []) - if so, not an array */
  token_t next_token = TRY_LOCAL(onerror, vec_get(tokens, current));
  if (token_is(next_token, CUBEC_TOKEN_SYMBOL, "]")) {
    /* This is a slice, not an array - return NULL so slice parser handles it */
    return NULL;
  }

  start_location = *token_get_location(open_bracket);
  start_location.filename = filename;

  /* Parse the array size expression using read_expression */
  size = read_expression(allocator, tokens, &current, filename);
  if (!size) {
    THROW_LOCAL(onerror, "expected expression for array size");
  }

  /* Skip whitespace before checking for ']' */
  skip_whitespace(tokens, &current);

  /* Expect ']' after the size expression */
  token_t close_bracket = TRY_LOCAL(onerror, vec_get(tokens, current));
  if (!token_is(close_bracket, CUBEC_TOKEN_SYMBOL, "]")) {
    /* Not an array declaration — restore position and return NULL.
       This allows the caller to try other parsers (e.g. [0; 64] is not valid). */
    allocator_free(allocator, &size);
    *position = current;
    return NULL;
  }
  current++;

  /* Parse the underlying type using read_expression_type (greedy).
   * The array declaration greedily consumes the full type expression,
   * including ternary: [N]a ? b : c → array(ternary(a, b, c)).
   * Use grouping for the alternative: ([N] a) ? b : c → ternary(array(a), b, c).
   * Namespace access binds tighter: [N]std::vec::Vec → [N](std::vec::Vec). */
  skip_whitespace(tokens, &current);
  type = read_expression_base(allocator, tokens, &current, filename);
  if (!type) {
    THROW_LOCAL(onerror, "expected type after array declaration");
  }

  node = TRY_LOCAL(onerror, allocator_create(allocator, &g_cubec_declaration_array_type,
                          &(cubec_declaration_array_init_t){
                              .size = size,
                              .type = type,
                          }));

  /* Set location from start to end of type */
  node->super.super.super.location = start_location;
  node->super.super.super.location.end = type->location.end;

  *position = current;
  return (node_t)node;

onerror:
  allocator_free(allocator, &size);
  allocator_free(allocator, &type);
  allocator_free(allocator, &node);
  return NULL;
}

/* --------------------------------------------------------------------------
 *  Factory: cubec_ast_create_array_type
 * -------------------------------------------------------------------------- */

node_t cubec_ast_create_array_type(allocator_t alloc, location_t loc,
                                   node_t size, node_t base) {
  cubec_declaration_array_init_t init = {.location = loc, .parent = NULL,
                                         .size = size, .type = base};
  return (node_t)allocator_create(alloc, &g_cubec_declaration_array_type,
                                  &init);
}
