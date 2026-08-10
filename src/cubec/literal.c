#include "cubec/literal.h"

static void _cubec_literal_init(cubec_literal_t self, allocator_t allocator,
                                cubec_literal_init_t *init) {
  if (!init) return;
  cubec_expression_init_t super_init = {
      .kind = init->kind,
      .parent = NULL,
  };
  super_init.location = init->location;
  g_cubec_expression_class.init(&self->super, allocator, &super_init);
}

static void _cubec_literal_dispose(cubec_literal_t self,
                                   allocator_t allocator) {
  g_cubec_expression_class.dispose(&self->super, allocator);
}

static void _cubec_literal_clone(cubec_literal_t self, allocator_t allocator,
                                 cubec_literal_t another) {
  g_cubec_expression_class.clone(&self->super, allocator, &another->super);
}

static void _cubec_literal_move(cubec_literal_t self, allocator_t allocator,
                                cubec_literal_t another) {
  g_cubec_expression_class.move(&self->super, allocator, &another->super);
  return;

cleanup:
  return;
}

class_t g_cubec_literal_class = {
    .name = "cubec.cubec.literal",
    .size = sizeof(struct _cubec_literal_t),
    .init = (class_init_fn_t)_cubec_literal_init,
    .dispose = (class_dispose_fn_t)_cubec_literal_dispose,
    .clone = (class_clone_fn_t)_cubec_literal_clone,
    .move = (class_move_fn_t)_cubec_literal_move,
};
