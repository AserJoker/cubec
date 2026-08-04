#include "engine/module.h"
#include <stdlib.h>
#include <string.h>

static void _module_init(void *self, allocator_t allocator, void *arg) {
  module_t mod = (module_t)self;
  (void)arg;
  mod->allocator = allocator;
  mod->filename = NULL;
  mod->source = NULL;
  mod->tokens = NULL;
  mod->program = NULL;
}

static void _module_dispose(void *self, allocator_t allocator) {
  module_t mod = (module_t)self;
  allocator_free(allocator, &mod->program);
  allocator_free(allocator, &mod->tokens);
  free(mod->source);
  free((void *)mod->filename);
}

type_t g_module_type = {
    .size = sizeof(struct _module_t),
    .name = "cubec.engine.module",
    .init = (type_init_fn_t)_module_init,
    .dispose = (type_dispose_fn_t)_module_dispose,
};

module_t module_create(allocator_t allocator, const char *filename,
                       const char *source, vec_t tokens, node_t program) {
  module_t mod = (module_t)allocator_create(allocator, &g_module_type, NULL);
  mod->filename = strdup(filename);
  mod->source = (char *)source;
  mod->tokens = tokens;
  mod->program = program;
  return mod;
}

void module_dispose(module_t mod) {
  allocator_free(mod->allocator, &mod);
}
