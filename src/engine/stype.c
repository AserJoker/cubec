#include "engine/stype.h"
#include <stdlib.h>
#include <string.h>

static void _stype_init(void *self, allocator_t allocator, void *arg) {
  (void)arg;
  stype_t type = (stype_t)self;
  type->header.allocator = allocator;
  type->header.kind = DEF_TYPE;
  type->header.node = NULL;
  type->instance.name = NULL;
  type->instance.size = 0;
  type->instance.align = 0;
  type->type_kind = TYPE_STRUCT;
  type->params = NULL;
  type->implements = NULL;
}

static void _stype_dispose(void *self, allocator_t allocator) {
  stype_t type = (stype_t)self;
  free(type->instance.name);
  type->instance.name = NULL;
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

stype_t stype_create_primitive(allocator_t allocator, enum type_kind_t kind,
                               const char *name, uint64_t size, uint64_t align) {
  stype_t type = stype_create(allocator, kind, NULL);
  type->instance.name = strdup(name);
  type->instance.size = size;
  type->instance.align = align;
  return type;
}

void stype_dispose(stype_t type) {
  allocator_free(type->header.allocator, &type);
}
