#include "engine/context.h"
#include "engine/module.h"
#include "engine/scope.h"
#include <string.h>

static void _context_init(void *self, allocator_t allocator, void *arg) {
  (void)arg;
  context_t ctx = (context_t)self;
  memset(ctx, 0, sizeof(struct context));
  ctx->allocator = allocator;

  diagnostic_list_init_t dl_init = {.output = NULL};
  ctx->diagnostics = (diagnostic_list_t)allocator_create(
      allocator, &g_diagnostic_list_type, &dl_init);

  strmap_init_t sm_init = {.value_auto_dispose = true};
  ctx->modules =
      (strmap_t)allocator_create(allocator, &g_strmap_type, &sm_init);

  ctx->global_scope = scope_create(allocator, SCOPE_GLOBAL, NULL, NULL);
  ctx->root_scope = NULL;
  ctx->current_scope = NULL;
}

static void _context_dispose(void *self, allocator_t allocator) {
  context_t ctx = (context_t)self;
  (void)allocator;
  allocator_free(allocator, &ctx->global_scope);
  allocator_free(allocator, &ctx->modules);
  allocator_free(allocator, &ctx->diagnostics);
}

type_t g_context_type = {
    .size = sizeof(struct context),
    .name = "cubec.engine.context",
    .init = (type_init_fn_t)_context_init,
    .dispose = (type_dispose_fn_t)_context_dispose,
};

context_t context_create(allocator_t allocator) {
  return (context_t)allocator_create(allocator, &g_context_type, NULL);
}

void context_dispose(context_t ctx) {
  allocator_free(ctx->allocator, &ctx);
}

int context_get_error_count(context_t ctx) {
  if (!ctx) return 0;
  return (int)diagnostic_list_get_error_count(ctx->diagnostics);
}

module_t context_get_module(context_t ctx, const char *abs_path) {
  return (module_t)strmap_find(ctx->modules, abs_path);
}
