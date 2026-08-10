#include "engine/module.h"
#include "engine/scope.h"
#include "core/string.h"
#include <stdlib.h>
#include <string.h>

static void _module_init(void *self, allocator_t allocator, void *arg) {
  (void)arg;
  module_t mod = (module_t)self;
  mod->allocator = allocator;
  mod->filename = NULL;
  mod->source = NULL;
  mod->tokens = NULL;
  mod->program = NULL;
  mod->root_scope = NULL;
  mod->exports = NULL;
  mod->state = MODULE_NEW;
}

static void _module_dispose(void *self, allocator_t allocator) {
  module_t mod = (module_t)self;
  (void)allocator;
  allocator_free(mod->allocator, &mod->root_scope);
  allocator_free(mod->allocator, &mod->exports);
  allocator_free(mod->allocator, &mod->program);
  allocator_free(mod->allocator, &mod->tokens);
  allocator_free(mod->allocator, (void **)&mod->source);
  allocator_free(mod->allocator, (void **)&mod->filename);
}

class_t g_module_class = {
    .size = sizeof(struct _module_t),
    .name = "cubec.engine.module",
    .init = (class_init_fn_t)_module_init,
    .dispose = (class_dispose_fn_t)_module_dispose,
};

module_t module_create(allocator_t allocator, scope_t parent_scope,
                       const char *filename, const char *source,
                       vec_t tokens, node_t program) {
  module_t mod = (module_t)allocator_create(allocator, &g_module_class, NULL);
  mod->filename = cstring_clone(allocator, filename);
  mod->source = (char *)source;
  mod->tokens = tokens;
  mod->program = program;
  /* scope_create already calls scope_add_child(parent_scope, scope) */
  mod->root_scope = scope_create(allocator, SCOPE_MODULE, parent_scope, mod);
  strmap_init_t sm_init = {.value_auto_dispose = false};
  mod->exports = (strmap_t)allocator_create(allocator, &g_strmap_class, &sm_init);
  return mod;
}

void module_dispose(module_t mod) {
  allocator_free(mod->allocator, &mod);
}
