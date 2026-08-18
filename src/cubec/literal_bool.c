#include "cubec/literal_bool.h"
#include "core/emit_context.h"
#include "core/token.h"
#include "cubec/token.h"

static void _cubec_literal_bool_init(cubec_literal_bool_t self,
                                     allocator_t allocator,
                                     cubec_literal_init_t *init) {
  if (!init)
    return;
  g_cubec_literal_class.init(&self->super, allocator, init);
}

static void _cubec_literal_bool_dispose(cubec_literal_bool_t self,
                                        allocator_t allocator) {
  g_cubec_literal_class.dispose(&self->super, allocator);
}

static void _cubec_literal_bool_clone(cubec_literal_bool_t self,
                                      allocator_t allocator,
                                      cubec_literal_bool_t another) {
  g_cubec_literal_class.clone(&self->super, allocator, &another->super);
  self->value = another->value;
}

static void _cubec_literal_bool_move(cubec_literal_bool_t self,
                                     allocator_t allocator,
                                     cubec_literal_bool_t another) {
  g_cubec_literal_class.move(&self->super, allocator, &another->super);
  self->value = another->value;
}

class_t g_cubec_literal_bool_class = {
    .name = "cubec.cubec.literal_bool",
    .size = sizeof(struct _cubec_literal_bool_t),
    .init = (class_init_fn_t)_cubec_literal_bool_init,
    .dispose = (class_dispose_fn_t)_cubec_literal_bool_dispose,
    .clone = (class_clone_fn_t)_cubec_literal_bool_clone,
    .move = (class_move_fn_t)_cubec_literal_bool_move,
};

node_t read_literal_bool(vm_t vm, vec_t tokens, size_t *position,
                         const char *filename) {
  allocator_t allocator = vm_get_allocator(vm);
  size_t current = *position;

  token_t token = vec_get(tokens, current);
  if (!token)
    return NULL;

  bool value;
  if (token_is(token, CUBEC_TOKEN_KEYWORD, "true"))
    value = true;
  else if (token_is(token, CUBEC_TOKEN_KEYWORD, "false"))
    value = false;
  else
    return NULL;

  location_t loc = *token_get_location(token);
  loc.filename = filename;
  current++;

  cubec_literal_init_t init = {
      .kind = CUBEC_NODE_LITERAL_BOOL,
      .parent = NULL,
  };
  init.location = loc;

  cubec_literal_bool_t node =
      allocator_create(allocator, &g_cubec_literal_bool_class, &init);
  if (node)
    node->value = value;
  *position = current;
  return (node_t)node;
}

node_t create_literal_bool(vm_t vm, location_t loc, bool value) {
  allocator_t alloc = vm_get_allocator(vm);
  cubec_literal_init_t init = {
      .kind = CUBEC_NODE_LITERAL_BOOL, .location = loc, .parent = NULL};
  cubec_literal_bool_t node =
      allocator_create(alloc, &g_cubec_literal_bool_class, &init);
  if (node)
    node->value = value;
  return (node_t)node;
}

void emit_literal_bool(emit_context_t ctx, node_t node) {
  cubec_literal_bool_t self = (cubec_literal_bool_t)node;
  recover_comments_to(ctx, node->location.begin.offset);
  emit_keyword(ctx, self->value ? "true" : "false");
}
