#include "engine/namespace.h"

static void _namespace_init(void *self, allocator_t allocator, void *arg) {
  (void)arg;
  namespace_t ns = (namespace_t)self;
  ns->header.allocator = allocator;
  ns->header.kind = DEF_NAMESPACE;
  ns->header.node = NULL;
  ns->module = NULL;
}

static void _namespace_dispose(void *self, allocator_t allocator) {
  (void)self;
  (void)allocator;
  /* module is borrowed — not freed here */
}

type_t g_namespace_type = {
    .size = sizeof(struct _namespace_t),
    .name = "cubec.engine.namespace",
    .init = (type_init_fn_t)_namespace_init,
    .dispose = (type_dispose_fn_t)_namespace_dispose,
};

namespace_t namespace_create(allocator_t allocator, node_t node) {
  namespace_t ns = (namespace_t)allocator_create(allocator, &g_namespace_type, NULL);
  ns->header.kind = DEF_NAMESPACE;
  ns->header.node = node;
  return ns;
}

void namespace_dispose(namespace_t ns) {
  allocator_free(ns->header.allocator, &ns);
}
