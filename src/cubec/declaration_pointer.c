#include "cubec/declaration_pointer.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/token.h"
#include "cubec/expression.h"
#include "cubec/node.h"
#include "cubec/token.h"

static void _cubec_declaration_pointer_init(cubec_declaration_pointer_t self,
                                            allocator_t allocator,
                                            cubec_declaration_pointer_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  cubec_declaration_init_t super_init = {
      .kind = CUBEC_NODE_DECLARATION_POINTER,
      .parent = NULL,
  };
  super_init.location = init->location;
  TRY_VOID_LOCAL(onerror, g_cubec_declaration_type.init(&self->super, allocator, &super_init));
  self->type = init->type;
  self->is_const = init->is_const;
  self->is_volatile = init->is_volatile;
onerror:
  return;
}

static void _cubec_declaration_pointer_dispose(cubec_declaration_pointer_t self,
                                               allocator_t allocator) {
  allocator_free(allocator, &self->type);
  g_cubec_declaration_type.dispose(&self->super, allocator);
}

static void _cubec_declaration_pointer_clone(cubec_declaration_pointer_t self,
                                             allocator_t allocator,
                                             cubec_declaration_pointer_t another) {
  TRY_VOID_LOCAL(cleanup, g_cubec_declaration_type.clone(&self->super, allocator, &another->super));
  self->type = TRY_LOCAL(cleanup, value_clone(allocator, another->type));
  self->is_const = another->is_const;
  self->is_volatile = another->is_volatile;
  return;

cleanup:
  allocator_free(allocator, &self->type);
}

static void _cubec_declaration_pointer_move(cubec_declaration_pointer_t self,
                                            allocator_t allocator,
                                            cubec_declaration_pointer_t another) {
  TRY_VOID_LOCAL(cleanup, g_cubec_declaration_type.move(&self->super, allocator, &another->super));
  self->type = TRY_LOCAL(cleanup, value_move(allocator, another->type));
  self->is_const = another->is_const;
  self->is_volatile = another->is_volatile;
  return;

cleanup:
  allocator_free(allocator, &self->type);
}

type_t g_cubec_declaration_pointer_type = {
    .name = "cubec.cubec.declaration_pointer",
    .size = sizeof(struct _cubec_declaration_pointer_t),
    .init = (type_init_fn_t)_cubec_declaration_pointer_init,
    .dispose = (type_dispose_fn_t)_cubec_declaration_pointer_dispose,
    .clone = (type_clone_fn_t)_cubec_declaration_pointer_clone,
    .move = (type_move_fn_t)_cubec_declaration_pointer_move,
};

/**
 * Helper function to check if a token is a specific keyword.
 */
static bool _is_keyword(vec_t tokens, size_t position, const char *keyword) {
  token_t token = vec_get(tokens, position);
  if (!token) return false;
  if (token_get_kind(token) != CUBEC_TOKEN_KEYWORD) return false;
  return location_is(token_get_location(token), keyword);
}

node_t read_declaration_pointer(allocator_t allocator, vec_t tokens,
                                size_t *position, const char *filename) {
  size_t current = *position;
  cubec_declaration_pointer_t node = NULL;
  node_t type = NULL;
  location_t start_location = {0};
  bool is_const = false;
  bool is_volatile = false;

  /* Expect '*' (pointer indicator) */
  token_t star_token = TRY_LOCAL(onerror, vec_get(tokens, current));
  if (!token_is(star_token, CUBEC_TOKEN_SYMBOL, "*")) {
    return NULL;
  }
  current++;
  start_location = *token_get_location(star_token);
  start_location.filename = filename;

  /* Skip whitespace after '*' */
  skip_whitespace(tokens, &current);

  /* Check for optional 'const' and 'volatile' qualifiers (any order, may repeat) */
  while (true) {
    if (_is_keyword(tokens, current, "const")) {
      is_const = true;
      current++;
      skip_whitespace(tokens, &current);
      continue;
    }
    if (_is_keyword(tokens, current, "volatile")) {
      is_volatile = true;
      current++;
      skip_whitespace(tokens, &current);
      continue;
    }
    break;
  }

  /* Parse the underlying type using read_type_expression_primary.
   * Ternary type expressions are not allowed directly as pointer base type;
   * use type_group to wrap them: *(a ? b : c) instead of * a ? b : c. */
  type = TRY_LOCAL(onerror, read_type_expression_primary(allocator, tokens, &current, filename));
  if (!type) {
    THROW_LOCAL(onerror, "expected type after pointer declaration");
  }

  node = TRY_LOCAL(onerror, allocator_create(allocator, &g_cubec_declaration_pointer_type,
                          &(cubec_declaration_pointer_init_t){
                              .type = type,
                              .is_const = is_const,
                              .is_volatile = is_volatile,
                          }));

  /* Set location from start to end of type */
  node->super.super.super.location = start_location;
  node->super.super.super.location.end = type->location.end;

  *position = current;
  return (node_t)node;

onerror:
  allocator_free(allocator, &type);
  allocator_free(allocator, &node);
  return NULL;
}