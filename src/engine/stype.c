#include "engine/stype.h"

static void _stype_init(void *self, allocator_t allocator, void *arg) {
  (void)arg;
  stype_t type = (stype_t)self;
  type->header.allocator = allocator;
  type->header.kind = DEF_TYPE;
  type->header.node = NULL;
  type->type_kind = TYPE_STRUCT;
  type->params = NULL;
  type->implements = NULL;
}

static void _stype_dispose(void *self, allocator_t allocator) {
  stype_t type = (stype_t)self;
  if (type->params)
    allocator_free(allocator, &type->params);
  if (type->implements)
    allocator_free(allocator, &type->implements);
}

type_t g_stype_type = {
    .size = sizeof(struct _stype_t),
    .name = "cubec.engine.stype",
    .init = (type_init_fn_t)_stype_init,
    .dispose = (type_dispose_fn_t)_stype_dispose,
};

stype_t stype_create(allocator_t allocator, enum type_kind_t kind, node_t node) {
  stype_t type = (stype_t)allocator_create(allocator, &g_stype_type, NULL);
  type->header.kind = DEF_TYPE;
  type->header.node = node;
  type->type_kind = kind;
  return type;
}

void stype_dispose(stype_t type) {
  allocator_free(type->header.allocator, &type);
}
