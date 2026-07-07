#include "cubec/declaration.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/node.h"

static void _cubec_declaration_init(cubec_declaration_t self,
                                    allocator_t allocator,
                                    cubec_declaration_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  cubec_expression_init_t super_init = {
      .kind = init->kind,
      .parent = NULL,
  };
  super_init.location = init->location;
  TRY_VOID_LOCAL(onerror, g_cubec_expression_type.init(&self->super, allocator, &super_init));
onerror:
  return;
}

static void _cubec_declaration_dispose(cubec_declaration_t self,
                                       allocator_t allocator) {
  g_cubec_expression_type.dispose(&self->super, allocator);
}

static void _cubec_declaration_clone(cubec_declaration_t self,
                                     allocator_t allocator,
                                     cubec_declaration_t another) {
  TRY_VOID_LOCAL(onerror, g_cubec_expression_type.clone(&self->super, allocator, &another->super));
onerror:
  return;
}

static void _cubec_declaration_move(cubec_declaration_t self,
                                    allocator_t allocator,
                                    cubec_declaration_t another) {
  TRY_VOID_LOCAL(onerror, g_cubec_expression_type.move(&self->super, allocator, &another->super));
onerror:
  return;
}

type_t g_cubec_declaration_type = {
    .name = "cubec.cubec.declaration",
    .size = sizeof(struct _cubec_declaration_t),
    .init = (type_init_fn_t)_cubec_declaration_init,
    .dispose = (type_dispose_fn_t)_cubec_declaration_dispose,
    .clone = (type_clone_fn_t)_cubec_declaration_clone,
    .move = (type_move_fn_t)_cubec_declaration_move,
};