#include "cubec/literal_string.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/node.h"
#include "core/string.h"
#include "core/token.h"
#include "core/type.h"
#include "cubec/literal.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include <stdio.h>

static void _cubec_literal_string_init(cubec_literal_string_t self,
                                       allocator_t allocator,
                                       cubec_literal_string_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  cubec_literal_init_t super_init = {
      .kind = CUBEC_NODE_LITERAL_STRING,
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

static void _cubec_literal_string_dispose(cubec_literal_string_t self,
                                          allocator_t allocator) {
  if (self->value) {
    allocator_free(allocator, &self->value);
  }
  g_cubec_literal_type.dispose(&self->super, allocator);
}

static void _cubec_literal_string_clone(cubec_literal_string_t self,
                                        allocator_t allocator,
                                        cubec_literal_string_t another) {
  g_cubec_literal_type.clone(&self->super, allocator, &another->super);
  self->value = TRY_LOCAL(cleanup, value_clone(allocator, another->value));
  return;

cleanup:
  allocator_free(allocator, &self->value);
}

static void _cubec_literal_string_move(cubec_literal_string_t self,
                                       allocator_t allocator,
                                       cubec_literal_string_t another) {
  TRY_VOID_LOCAL(cleanup, g_cubec_literal_type.move(&self->super, allocator, &another->super));
  self->value = TRY_LOCAL(cleanup, value_move(allocator, another->value));
  return;

cleanup:
  allocator_free(allocator, &self->value);
}

type_t g_cubec_literal_string_type = {
    .name = "cubec.cubec.literal_string",
    .size = sizeof(struct _cubec_literal_string_t),
    .init = (type_init_fn_t)_cubec_literal_string_init,
    .dispose = (type_dispose_fn_t)_cubec_literal_string_dispose,
    .clone = (type_clone_fn_t)_cubec_literal_string_clone,
    .move = (type_move_fn_t)_cubec_literal_string_move,
};

node_t read_literal_string(allocator_t allocator, vec_t tokens, size_t *position,
                           const char *filename) {
  size_t current = *position;

  token_t first_token = TRY_LOCAL(onerror, vec_get(tokens, current));
  if (!token_is(first_token, CUBEC_TOKEN_STRING, NULL)) {
    return NULL;
  }

  location_t *location = token_get_location(first_token);
  cubec_literal_string_init_t init = {
      .location = *location,
      .parent = NULL,
      .value = NULL,
  };
  cubec_literal_string_t node =
      TRY_LOCAL(onerror, allocator_create(allocator, &g_cubec_literal_string_type, &init));
  node_t node_base = (node_t)node;
  node_base->location.filename = filename;

  const char *token_str = token_get_string(first_token);
  size_t token_len = token_get_string_length(first_token);
  if (token_len >= 2) {
    string_nconcat(node->value, token_str + 1, token_len - 2);
  }
  current++;

  while (true) {
    skip_whitespace(tokens, &current);
    token_t token = TRY_LOCAL(onerror, vec_get(tokens, current));
    if (!token_is(token, CUBEC_TOKEN_STRING, NULL)) {
      break;
    }
    token_str = token_get_string(token);
    token_len = token_get_string_length(token);
    if (token_len >= 2) {
      string_nconcat(node->value, token_str + 1, token_len - 2);
    }
    location_t *token_location = token_get_location(token);
    node_base->location.end = token_location->end;
    current++;
  }

  *position = current;
  return node_base;
onerror:
  allocator_free(allocator, &node);
  return NULL;
}