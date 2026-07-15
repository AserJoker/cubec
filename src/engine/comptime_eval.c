#include "engine/comptime_eval.h"
#include "engine/comptime_eval_internal.h"
#include "core/allocator.h"
#include "core/vec.h"

comptime_eval_t comptime_eval_create(allocator_t allocator) {
  comptime_eval_t eval =
      (comptime_eval_t)allocator_alloc(allocator, sizeof(struct comptime_eval));
  if (!eval) return NULL;
  eval->allocator = allocator;
  eval->valloc = comptime_allocator_create(allocator);
  eval->global_env = comptime_env_create(allocator, NULL);
  eval->current_env = eval->global_env;
  eval->call_depth = 0;
  eval->loop_depth = 0;
  vec_init_t vi = {.auto_dispose = false};
  eval->defer_stack = (vec_t)allocator_create(allocator, &g_vec_type, &vi);

  /* Bind builtin literal values in global environment */
  comptime_env_bind(eval->global_env, "true",
      comptime_value_create_bool(allocator, true, NULL));
  comptime_env_bind(eval->global_env, "false",
      comptime_value_create_bool(allocator, false, NULL));
  comptime_env_bind(eval->global_env, "nil",
      comptime_value_create_nil(allocator, NULL));

  return eval;
}

void comptime_eval_dispose(comptime_eval_t self) {
  if (!self) return;
  allocator_t a = self->allocator;
  vec_resize(self->defer_stack, 0);
  allocator_free(a, &self->defer_stack);
  comptime_env_dispose(self->global_env);
  comptime_allocator_dispose(self->valloc);
  /* Do NOT free self here — _checker_dispose calls allocator_free for the eval struct */
}
