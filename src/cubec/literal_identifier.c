#include "cubec/literal_identifier.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/node.h"
#include "core/string.h"
#include "core/token.h"
#include "core/type.h"
#include "cubec/literal.h"
#include "cubec/token.h"

static void _cubec_literal_identifier_init(cubec_literal_identifier_t self,
                                           allocator_t allocator,
                                           cubec_literal_identifier_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  cubec_literal_init_t super_init = {
      .kind = CUBEC_NODE_LITERAL_IDENTIFIER,
      .parent = NULL,
  };
  super_init.location = init->location;
  TRY_VOID_LOCAL(onerror, g_cubec_literal_type.init(&self->super, allocator, &super_init));
  if (init->value) {
    self->value = allocator_create(allocator, &g_string_type,
                                   &(string_init_t){.str = init->value});
  } else {
    self->value = allocator_create(allocator, &g_string_type, NULL);
  }
onerror:
  return;
}

static void _cubec_literal_identifier_dispose(cubec_literal_identifier_t self,
                                              allocator_t allocator) {
  if (self->value) {
    allocator_free(allocator, &self->value);
  }
  g_cubec_literal_type.dispose(&self->super, allocator);
}

static void _cubec_literal_identifier_clone(cubec_literal_identifier_t self,
                                            allocator_t allocator,
                                            cubec_literal_identifier_t another) {
  g_cubec_literal_type.clone(&self->super, allocator, &another->super);
  self->value = TRY_LOCAL(cleanup, value_clone(allocator, another->value));
  return;

cleanup:
  allocator_free(allocator, &self->value);
}

static void _cubec_literal_identifier_move(cubec_literal_identifier_t self,
                                           allocator_t allocator,
                                           cubec_literal_identifier_t another) {
  g_cubec_literal_type.move(&self->super, allocator, &another->super);
  self->value = TRY_LOCAL(cleanup, value_move(allocator, another->value));
  return;

cleanup:
  allocator_free(allocator, &self->value);
}

type_t g_cubec_literal_identifier_type = {
    .name = "cubec.cubec.literal_identifier",
    .size = sizeof(struct _cubec_literal_identifier_t),
    .init = (type_init_fn_t)_cubec_literal_identifier_init,
    .dispose = (type_dispose_fn_t)_cubec_literal_identifier_dispose,
    .clone = (type_clone_fn_t)_cubec_literal_identifier_clone,
    .move = (type_move_fn_t)_cubec_literal_identifier_move,
};

node_t read_literal_identifier(allocator_t allocator, vec_t tokens,
                               size_t *position, const char *filename) {
  size_t current = *position;

  token_t token = TRY_LOCAL(onerror, vec_get(tokens, current));
  if (!token_is(token, CUBEC_TOKEN_IDENTIFIER, NULL)) {
    return NULL;
  }

  location_t *location = token_get_location(token);
  cubec_literal_identifier_init_t init = {
      .location = *location,
      .parent = NULL,
      .value = NULL,
  };
  cubec_literal_identifier_t node =
      TRY_LOCAL(onerror, allocator_create(allocator, &g_cubec_literal_identifier_type, &init));
  node_t node_base = (node_t)node;
  node_base->location.filename = filename;

  const char *token_str = token_get_string(token);
  size_t token_len = token_get_string_length(token);
  string_nconcat(node->value, token_str, token_len);
  current++;

  *position = current;
  return node_base;
onerror:
  allocator_free(allocator, &node);
  return NULL;
}