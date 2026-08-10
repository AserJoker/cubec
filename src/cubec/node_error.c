#include "cubec/node_error.h"
#include "core/emit_context.h"
#include "core/token_writer.h"
#include "engine/context.h"

static void _cubec_node_error_init(cubec_node_error_t self,
                                   allocator_t allocator,
                                   cubec_node_error_init_t *init) {
  if (!init)
    return;
  node_init_t super_init = {
      .kind = CUBEC_NODE_ERROR,
      .parent = NULL,
  };
  super_init.location = init->location;
  g_node_class.init(&self->super, allocator, &super_init);
}

static void _cubec_node_error_dispose(cubec_node_error_t self,
                                      allocator_t allocator) {
  g_node_class.dispose(&self->super, allocator);
}

static void _cubec_node_error_clone(cubec_node_error_t self,
                                    allocator_t allocator,
                                    cubec_node_error_t another) {
  g_node_class.clone(&self->super, allocator, &another->super);
}

static void _cubec_node_error_move(cubec_node_error_t self,
                                   allocator_t allocator,
                                   cubec_node_error_t another) {
  g_node_class.move(&self->super, allocator, &another->super);
}

class_t g_cubec_node_error_class = {
    .name = "cubec.cubec.node_error",
    .size = sizeof(struct _cubec_node_error_t),
    .init = (class_init_fn_t)_cubec_node_error_init,
    .dispose = (class_dispose_fn_t)_cubec_node_error_dispose,
    .clone = (class_clone_fn_t)_cubec_node_error_clone,
    .move = (class_move_fn_t)_cubec_node_error_move,
};

node_t create_error(context_t ctx, location_t loc) {
  allocator_t alloc = ctx->allocator;
  cubec_node_error_init_t init = {.location = loc, .parent = NULL};
  return (node_t)allocator_create(alloc, &g_cubec_node_error_class, &init);
}

void emit_node_error(emit_context_t ctx, node_t node) {
  (void)node;
  emit_symbol(ctx, "/* error */");
}

