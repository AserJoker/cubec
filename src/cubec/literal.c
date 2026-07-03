#include "cubec/literal.h"
#include "core/allocator.h"
#include "core/type.h"
#include "cubec/expression.h"

static void _cubec_literal_init(cubec_literal_t self, allocator_t allocator,
                                cubec_literal_init_t *init) {
  cubec_expression_init_t super_init = {
      .kind = 0,
      .parent = NULL,
  };
  if (init) {
    super_init.location = init->location;
    super_init.kind = init->kind;
  }
  g_cubec_expression_type.init(&self->super, allocator, &super_init);
}

static void _cubec_literal_dispose(cubec_literal_t self,
                                   allocator_t allocator) {
  g_cubec_expression_type.dispose(&self->super, allocator);
}

static void _cubec_literal_clone(cubec_literal_t self, allocator_t allocator,
                                 cubec_literal_t another) {
  g_cubec_expression_type.clone(&self->super, allocator, &another->super);
}

static void _cubec_literal_move(cubec_literal_t self, allocator_t allocator,
                                cubec_literal_t another) {
  g_cubec_expression_type.move(&self->super, allocator, &another->super);
}

type_t g_cubec_literal_type = {
    .name = "cubec.cubec.literal",
    .size = sizeof(struct _cubec_literal_t),
    .init = (type_init_fn_t)_cubec_literal_init,
    .dispose = (type_dispose_fn_t)_cubec_literal_dispose,
    .clone = (type_clone_fn_t)_cubec_literal_clone,
    .move = (type_move_fn_t)_cubec_literal_move,
};
