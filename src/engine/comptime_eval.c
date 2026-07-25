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
  eval->eval_expr_count = 0;
  vec_init_t vi = {.auto_dispose = false};
  eval->cleanup_stack = (vec_t)allocator_create(allocator, &g_vec_type, &vi);
  vec_init_t cvi = {.auto_dispose = false};
  eval->captured_envs = (vec_t)allocator_create(allocator, &g_vec_type, &cvi);
  vec_init_t rti = {.auto_dispose = false};
  eval->return_type_stack = (vec_t)allocator_create(allocator, &g_vec_type, &rti);
  eval->propagated_return = false;
  eval->propagated_return_value = NULL;

  /* Bind builtin literal values in global environment (allocated at scope depth 0) */
  comptime_env_bind_value(eval->global_env, eval->valloc, "true",
      comptime_value_create_bool(allocator, true, NULL));
  comptime_env_bind_value(eval->global_env, eval->valloc, "false",
      comptime_value_create_bool(allocator, false, NULL));
  comptime_env_bind_value(eval->global_env, eval->valloc, "nil",
      comptime_value_create_nil(allocator, NULL));

  return eval;
}

void comptime_eval_dispose(comptime_eval_t self) {
  if (!self) return;
  allocator_t a = self->allocator;

  /* 1. Clear cleanup stack */
  vec_resize(self->cleanup_stack, 0);
  allocator_free(a, &self->cleanup_stack);

  /* 2. Dispose global env (frees temporaries including function values,
     but function values no longer dispose their captured_env themselves) */
  comptime_env_dispose(self->global_env);

  /* 3. Dispose comptime allocator (frees all allocated values) */
  comptime_allocator_dispose(self->valloc);

  /* 4. Dispose all captured envs (owned by the eval, not by individual
     function values — avoids double-free when function values are cloned) */
  size_t ce_count = vec_get_size(self->captured_envs);
  for (size_t i = 0; i < ce_count; i++) {
    comptime_env_t env = (comptime_env_t)vec_get(self->captured_envs, i);
    comptime_env_dispose(env);
  }
  allocator_free(a, &self->captured_envs);

  /* 5. Free return type stack */
  vec_resize(self->return_type_stack, 0);
  allocator_free(a, &self->return_type_stack);

  /* Do NOT free self here — _context_dispose calls allocator_free for the eval struct */
}
