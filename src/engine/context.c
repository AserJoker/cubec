#include "engine/context.h"
#include "engine/vm.h"
#include <string.h>

static void _context_init(void *self, allocator_t allocator, void *arg) {
  (void)arg;
  context_t ctx = (context_t)self;
  memset(ctx, 0, sizeof(struct context));
  ctx->allocator = allocator;

  ctx->vm = vm_create(allocator);

  diagnostic_list_init_t dl_init = {.output = NULL};
  ctx->diagnostics = (diagnostic_list_t)allocator_create(
      allocator, &g_diagnostic_list_class, &dl_init);
}

static void _context_dispose(void *self, allocator_t allocator) {
  context_t ctx = (context_t)self;
  (void)allocator;
  vm_dispose(ctx->vm, allocator);
  ctx->vm = NULL;
  allocator_free(allocator, &ctx->diagnostics);
}

class_t g_context_class = {
    .size = sizeof(struct context),
    .name = "cubec.engine.context",
    .init = (class_init_fn_t)_context_init,
    .dispose = (class_dispose_fn_t)_context_dispose,
};

context_t context_create(allocator_t allocator) {
  return (context_t)allocator_create(allocator, &g_context_class, NULL);
}

void context_dispose(context_t ctx) {
  allocator_free(ctx->allocator, &ctx);
}

int context_get_error_count(context_t ctx) {
  if (!ctx) return 0;
  return (int)diagnostic_list_get_error_count(ctx->diagnostics);
}
