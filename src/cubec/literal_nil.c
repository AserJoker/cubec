#include "cubec/literal_nil.h"
#include "core/emit_context.h"
#include "core/token.h"
#include "cubec/token.h"

static void _cubec_literal_nil_init(cubec_literal_nil_t self,
                                    allocator_t allocator,
                                    cubec_literal_init_t *init) {
  if (!init)
    return;
  g_cubec_literal_class.init(&self->super, allocator, init);
}

static void _cubec_literal_nil_dispose(cubec_literal_nil_t self,
                                       allocator_t allocator) {
  g_cubec_literal_class.dispose(&self->super, allocator);
}

static void _cubec_literal_nil_clone(cubec_literal_nil_t self,
                                     allocator_t allocator,
                                     cubec_literal_nil_t another) {
  g_cubec_literal_class.clone(&self->super, allocator, &another->super);
}

static void _cubec_literal_nil_move(cubec_literal_nil_t self,
                                    allocator_t allocator,
                                    cubec_literal_nil_t another) {
  g_cubec_literal_class.move(&self->super, allocator, &another->super);
}

class_t g_cubec_literal_nil_class = {
    .name = "cubec.cubec.literal_nil",
    .size = sizeof(struct _cubec_literal_nil_t),
    .init = (class_init_fn_t)_cubec_literal_nil_init,
    .dispose = (class_dispose_fn_t)_cubec_literal_nil_dispose,
    .clone = (class_clone_fn_t)_cubec_literal_nil_clone,
    .move = (class_move_fn_t)_cubec_literal_nil_move,
};

node_t read_literal_nil(vm_t vm, vec_t tokens, size_t *position,
                        const char *filename) {
  allocator_t allocator = vm_get_allocator(vm);
  size_t current = *position;

  token_t token = vec_get(tokens, current);
  if (!token_is(token, CUBEC_TOKEN_KEYWORD, "nil"))
    return NULL;

  location_t loc = *token_get_location(token);
  loc.filename = filename;
  current++;

  cubec_literal_init_t init = {
      .kind = CUBEC_NODE_LITERAL_NIL,
      .parent = NULL,
  };
  init.location = loc;

  cubec_literal_nil_t node =
      allocator_create(allocator, &g_cubec_literal_nil_class, &init);
  *position = current;
  return (node_t)node;
}

node_t create_literal_nil(vm_t vm, location_t loc) {
  allocator_t alloc = vm_get_allocator(vm);
  cubec_literal_init_t init = {
      .kind = CUBEC_NODE_LITERAL_NIL, .location = loc, .parent = NULL};
  return (node_t)allocator_create(alloc, &g_cubec_literal_nil_class, &init);
}

void emit_literal_nil(emit_context_t ctx, node_t node) {
  recover_comments_to(ctx, node->location.begin.offset);
  emit_keyword(ctx, "nil");
}
