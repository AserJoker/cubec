#include "cubec/literal_char.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/node.h"
#include "core/token.h"
#include "core/type.h"
#include "cubec/literal.h"
#include "cubec/token.h"

static void _cubec_literal_char_init(cubec_literal_char_t self,
                                     allocator_t allocator,
                                     cubec_literal_char_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  cubec_literal_init_t super_init = {
      .kind = CUBEC_NODE_LITERAL_CHAR,
      .parent = NULL,
  };
  super_init.location = init->location;
  self->value = init->value;
  g_cubec_literal_type.init(&self->super, allocator, &super_init);
onerror:
  return;
}

static void _cubec_literal_char_dispose(cubec_literal_char_t self,
                                        allocator_t allocator) {
  g_cubec_literal_type.dispose(&self->super, allocator);
}

static void _cubec_literal_char_clone(cubec_literal_char_t self,
                                      allocator_t allocator,
                                      cubec_literal_char_t another) {
  g_cubec_literal_type.clone(&self->super, allocator, &another->super);
  self->value = another->value;
}

static void _cubec_literal_char_move(cubec_literal_char_t self,
                                     allocator_t allocator,
                                     cubec_literal_char_t another) {
  g_cubec_literal_type.move(&self->super, allocator, &another->super);
  self->value = another->value;
}

type_t g_cubec_literal_char_type = {
    .name = "cubec.cubec.literal_char",
    .size = sizeof(struct _cubec_literal_char_t),
    .init = (type_init_fn_t)_cubec_literal_char_init,
    .dispose = (type_dispose_fn_t)_cubec_literal_char_dispose,
    .clone = (type_clone_fn_t)_cubec_literal_char_clone,
    .move = (type_move_fn_t)_cubec_literal_char_move,
};

node_t read_literal_char(allocator_t allocator, vec_t tokens,
                         size_t *position, const char *filename) {
  size_t current = *position;

  token_t token = TRY_LOCAL(onerror, vec_get(tokens, current));
  if (!token_is(token, CUBEC_TOKEN_CHAR, NULL)) {
    return NULL;
  }

  location_t *location = token_get_location(token);
  const char *token_str = token_get_string(token);
  size_t token_len = token_get_string_length(token);
  char value = 0;
  if (token_len >= 2) {
    value = token_str[1];
  }
  cubec_literal_char_init_t init = {
      .location = *location,
      .parent = NULL,
      .value = value,
  };
  cubec_literal_char_t node =
      TRY_LOCAL(onerror, allocator_create(allocator, &g_cubec_literal_char_type, &init));
  node_t node_base = (node_t)node;
  node_base->location.filename = filename;
  current++;

  *position = current;
  return node_base;
onerror:
  allocator_free(allocator, &node);
  return NULL;
}