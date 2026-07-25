#include "cubec/declaration.h"
#include "core/allocator.h"
#include "core/node.h"
#include "engine/context.h"

static void _cubec_declaration_init(cubec_declaration_t self,
                                    allocator_t allocator,
                                    cubec_declaration_init_t *init) {
  if (!init) return;
  cubec_expression_init_t super_init = {
      .kind = init->kind,
      .parent = NULL,
  };
  super_init.location = init->location;
  g_cubec_expression_type.init(&self->super, allocator, &super_init);
}

static void _cubec_declaration_dispose(cubec_declaration_t self,
                                       allocator_t allocator) {
  g_cubec_expression_type.dispose(&self->super, allocator);
}

static void _cubec_declaration_clone(cubec_declaration_t self,
                                     allocator_t allocator,
                                     cubec_declaration_t another) {
  g_cubec_expression_type.clone(&self->super, allocator, &another->super);
}

static void _cubec_declaration_move(cubec_declaration_t self,
                                    allocator_t allocator,
                                    cubec_declaration_t another) {
  g_cubec_expression_type.move(&self->super, allocator, &another->super);
}

type_t g_cubec_declaration_type = {
    .name = "cubec.cubec.declaration",
    .size = sizeof(struct _cubec_declaration_t),
    .init = (type_init_fn_t)_cubec_declaration_init,
    .dispose = (type_dispose_fn_t)_cubec_declaration_dispose,
    .clone = (type_clone_fn_t)_cubec_declaration_clone,
    .move = (type_move_fn_t)_cubec_declaration_move,
};