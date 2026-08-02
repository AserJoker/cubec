#include "cubec/statement_error.h"
#include "cubec/node.h"
#include "engine/context.h"

static void _cubec_statement_error_init(cubec_statement_error_t self,
                                        allocator_t allocator,
                                        cubec_statement_error_init_t *init) {
  if (!init)
    return;
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_ERROR,
      .parent = NULL,
  };
  super_init.location = init->location;
  g_node_type.init(&self->super, allocator, &super_init);
}

static void _cubec_statement_error_dispose(cubec_statement_error_t self,
                                           allocator_t allocator) {
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_statement_error_clone(cubec_statement_error_t self,
                                         allocator_t allocator,
                                         cubec_statement_error_t another) {
  g_node_type.clone(&self->super, allocator, &another->super);
}

static void _cubec_statement_error_move(cubec_statement_error_t self,
                                        allocator_t allocator,
                                        cubec_statement_error_t another) {
  g_node_type.move(&self->super, allocator, &another->super);
}

type_t g_cubec_statement_error_type = {
    .name = "cubec.cubec.statement_error",
    .size = sizeof(struct _cubec_statement_error_t),
    .init = (type_init_fn_t)_cubec_statement_error_init,
    .dispose = (type_dispose_fn_t)_cubec_statement_error_dispose,
    .clone = (type_clone_fn_t)_cubec_statement_error_clone,
    .move = (type_move_fn_t)_cubec_statement_error_move,
};

node_t create_statement_error(context_t ctx, location_t loc) {
  allocator_t alloc = ctx->allocator;
  cubec_statement_error_init_t init = {.location = loc, .parent = NULL};
  return (node_t)allocator_create(alloc, &g_cubec_statement_error_type, &init);
}
