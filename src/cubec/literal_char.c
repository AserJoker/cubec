#include "cubec/literal_char.h"
#include "core/emit_context.h"
#include "core/token.h"
#include "cubec/node_error.h"
#include "cubec/token.h"

static void _cubec_literal_char_init(cubec_literal_char_t self,
                                     allocator_t allocator,
                                     cubec_literal_char_init_t *init) {
  if (!init)
    return;
  cubec_literal_init_t super_init = {
      .kind = CUBEC_NODE_LITERAL_CHAR,
      .parent = NULL,
  };
  super_init.location = init->location;
  self->value = init->value;
  g_cubec_literal_class.init(&self->super, allocator, &super_init);
}

static void _cubec_literal_char_dispose(cubec_literal_char_t self,
                                        allocator_t allocator) {
  g_cubec_literal_class.dispose(&self->super, allocator);
}

static void _cubec_literal_char_clone(cubec_literal_char_t self,
                                      allocator_t allocator,
                                      cubec_literal_char_t another) {
  g_cubec_literal_class.clone(&self->super, allocator, &another->super);
  self->value = another->value;
}

static void _cubec_literal_char_move(cubec_literal_char_t self,
                                     allocator_t allocator,
                                     cubec_literal_char_t another) {
  g_cubec_literal_class.move(&self->super, allocator, &another->super);
  self->value = another->value;
  return;

cleanup:
  return;
}

class_t g_cubec_literal_char_class = {
    .name = "cubec.cubec.literal_char",
    .size = sizeof(struct _cubec_literal_char_t),
    .init = (class_init_fn_t)_cubec_literal_char_init,
    .dispose = (class_dispose_fn_t)_cubec_literal_char_dispose,
    .clone = (class_clone_fn_t)_cubec_literal_char_clone,
    .move = (class_move_fn_t)_cubec_literal_char_move,
};

node_t read_literal_char(context_t ctx, vec_t tokens, size_t *position,
                         const char *filename) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;

  token_t token = vec_get(tokens, current);
  if (!token_is(token, CUBEC_TOKEN_CHAR, NULL)) {
    return NULL;
  }

  location_t start_location = *token_get_location(token);
  start_location.filename = filename;

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
      allocator_create(allocator, &g_cubec_literal_char_class, &init);
  if (!node)
    goto onerror;
  node_t node_base = (node_t)node;
  node_base->location.filename = filename;
  current++;

  *position = current;
  return node_base;
onerror:
  diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, start_location,
                       "invalid character literal");
  allocator_free(allocator, &node);
  return create_error(ctx, start_location);
}

node_t create_literal_char(context_t ctx, location_t loc, char value) {
  allocator_t alloc = ctx->allocator;
  cubec_literal_char_init_t init = {
      .location = loc, .parent = NULL, .value = value};
  return (node_t)allocator_create(alloc, &g_cubec_literal_char_class, &init);
}

void emit_literal_char(emit_context_t ctx, node_t node) {
  cubec_literal_char_t ch = (cubec_literal_char_t)node;
  recover_comments_to(ctx, node->location.begin.offset);
  char buf[4] = {'\'', ch->value, '\'', '\0'};
  emit_char_literal(ctx, buf);
}