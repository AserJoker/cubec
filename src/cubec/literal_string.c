#include "cubec/literal_string.h"
#include "core/emit_context.h"
#include "core/string.h"
#include "core/token.h"
#include "core/token_writer.h"
#include "cubec/node_error.h"
#include "cubec/token.h"

static void _cubec_literal_string_init(cubec_literal_string_t self,
                                       allocator_t allocator,
                                       cubec_literal_string_init_t *init) {
  if (!init)
    return;
  cubec_literal_init_t super_init = {
      .kind = CUBEC_NODE_LITERAL_STRING,
      .parent = NULL,
  };
  super_init.location = init->location;
  g_cubec_literal_class.init(&self->super, allocator, &super_init);
  if (init->value) {
    self->value = allocator_create(allocator, &g_string_class,
                                   &(string_init_t){.str = init->value});
  } else {
    self->value = allocator_create(allocator, &g_string_class, NULL);
  }
}

static void _cubec_literal_string_dispose(cubec_literal_string_t self,
                                          allocator_t allocator) {
  if (self->value) {
    allocator_free(allocator, &self->value);
  }
  g_cubec_literal_class.dispose(&self->super, allocator);
}

static void _cubec_literal_string_clone(cubec_literal_string_t self,
                                        allocator_t allocator,
                                        cubec_literal_string_t another) {
  g_cubec_literal_class.clone(&self->super, allocator, &another->super);
  self->value = alloc_clone(allocator, another->value);
}

static void _cubec_literal_string_move(cubec_literal_string_t self,
                                       allocator_t allocator,
                                       cubec_literal_string_t another) {
  g_cubec_literal_class.move(&self->super, allocator, &another->super);
  self->value = alloc_move(allocator, another->value);
}

class_t g_cubec_literal_string_class = {
    .name = "cubec.cubec.literal_string",
    .size = sizeof(struct _cubec_literal_string_t),
    .init = (class_init_fn_t)_cubec_literal_string_init,
    .dispose = (class_dispose_fn_t)_cubec_literal_string_dispose,
    .clone = (class_clone_fn_t)_cubec_literal_string_clone,
    .move = (class_move_fn_t)_cubec_literal_string_move,
};

node_t read_literal_string(vm_t vm, vec_t tokens, size_t *position,
                           const char *filename) {
  allocator_t allocator = vm_get_allocator(vm);
  size_t current = *position;

  token_t first_token = vec_get(tokens, current);
  if (!token_is(first_token, CUBEC_TOKEN_STRING, NULL)) {
    return NULL;
  }

  location_t start_location = *token_get_location(first_token);
  start_location.filename = filename;

  location_t *location = token_get_location(first_token);
  cubec_literal_string_init_t init = {
      .location = *location,
      .parent = NULL,
      .value = NULL,
  };
  cubec_literal_string_t node =
      allocator_create(allocator, &g_cubec_literal_string_class, &init);
  if (!node)
    goto onerror;
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
    token_t token = vec_get(tokens, current);
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
  diagnostic_list_push(vm_get_diagnostics(vm), DIAGNOSTIC_ERROR, start_location,
                       "invalid string literal");
  allocator_free(allocator, &node);
  return create_error(vm, start_location);
}

node_t create_literal_string(vm_t vm, location_t loc, const char *value) {
  allocator_t alloc = vm_get_allocator(vm);
  cubec_literal_string_init_t init = {.value = value};
  return (node_t)allocator_create(alloc, &g_cubec_literal_string_class, &init);
}

void emit_literal_string(emit_context_t ctx, node_t node) {
  cubec_literal_string_t str = (cubec_literal_string_t)node;
  recover_comments_to(ctx, node->location.begin.offset);
  emit_symbol(ctx, "\"");
  emit_string_literal(ctx, string_get(str->value));
  emit_symbol(ctx, "\"");
}