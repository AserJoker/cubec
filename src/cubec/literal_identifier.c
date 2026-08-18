#include "cubec/literal_identifier.h"
#include "core/emit_context.h"
#include "core/token.h"
#include "cubec/node_error.h"
#include "cubec/token.h"

static void
_cubec_literal_identifier_init(cubec_literal_identifier_t self,
                               allocator_t allocator,
                               cubec_literal_identifier_init_t *init) {
  if (!init)
    return;
  cubec_literal_init_t super_init = {
      .kind = CUBEC_NODE_LITERAL_IDENTIFIER,
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

static void _cubec_literal_identifier_dispose(cubec_literal_identifier_t self,
                                              allocator_t allocator) {
  if (self->value) {
    allocator_free(allocator, &self->value);
  }
  g_cubec_literal_class.dispose(&self->super, allocator);
}

static void
_cubec_literal_identifier_clone(cubec_literal_identifier_t self,
                                allocator_t allocator,
                                cubec_literal_identifier_t another) {
  g_cubec_literal_class.clone(&self->super, allocator, &another->super);
  self->value = alloc_clone(allocator, another->value);
}

static void _cubec_literal_identifier_move(cubec_literal_identifier_t self,
                                           allocator_t allocator,
                                           cubec_literal_identifier_t another) {
  g_cubec_literal_class.move(&self->super, allocator, &another->super);
  self->value = alloc_move(allocator, another->value);
}

class_t g_cubec_literal_identifier_class = {
    .name = "cubec.cubec.literal_identifier",
    .size = sizeof(struct _cubec_literal_identifier_t),
    .init = (class_init_fn_t)_cubec_literal_identifier_init,
    .dispose = (class_dispose_fn_t)_cubec_literal_identifier_dispose,
    .clone = (class_clone_fn_t)_cubec_literal_identifier_clone,
    .move = (class_move_fn_t)_cubec_literal_identifier_move,
};

node_t read_literal_identifier(vm_t vm, vec_t tokens, size_t *position,
                               const char *filename) {
  allocator_t allocator = vm_get_allocator(vm);
  size_t current = *position;

  token_t token = vec_get(tokens, current);
  if (!token_is(token, CUBEC_TOKEN_IDENTIFIER, NULL)) {
    return NULL;
  }

  location_t start_location = *token_get_location(token);
  start_location.filename = filename;

  location_t *location = token_get_location(token);
  cubec_literal_identifier_init_t init = {
      .location = *location,
      .parent = NULL,
      .value = NULL,
  };
  cubec_literal_identifier_t node = NULL;
  node = allocator_create(allocator, &g_cubec_literal_identifier_class, &init);
  if (!node)
    goto onerror;
  node_t node_base = (node_t)node;
  node_base->location.filename = filename;

  const char *token_str = token_get_string(token);
  size_t token_len = token_get_string_length(token);
  string_nconcat(node->value, token_str, token_len);
  current++;

  *position = current;
  return node_base;
onerror:
  diagnostic_list_push(vm_get_diagnostics(vm), DIAGNOSTIC_ERROR, start_location,
                       "invalid identifier");
  allocator_free(allocator, &node);
  return create_error(vm, start_location);
}

node_t create_literal_identifier(vm_t vm, location_t loc,
                                 const char *name) {
  allocator_t alloc = vm_get_allocator(vm);
  cubec_literal_identifier_init_t init = {
      .location = loc, .parent = NULL, .value = name};
  return allocator_create(alloc, &g_cubec_literal_identifier_class, &init);
}

void emit_literal_identifier(emit_context_t ctx, node_t node) {
  cubec_literal_identifier_t identifier = (cubec_literal_identifier_t)node;
  recover_comments_to(ctx, node->location.begin.offset);
  emit_identifier(ctx, string_get(identifier->value));
}