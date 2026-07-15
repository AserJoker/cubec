#include "engine/comptime_eval.h"
#include "core/allocator.h"
#include "core/strmap.h"
#include "core/vec.h"

comptime_env_t comptime_env_create(allocator_t allocator, comptime_env_t parent) {
  comptime_env_t env =
      (comptime_env_t)allocator_alloc(allocator, sizeof(struct comptime_env));
  if (!env) return NULL;
  env->allocator = allocator;
  env->parent = parent;
  /* value_auto_dispose = true: env owns bound values, frees on dispose */
  strmap_init_t si = {.value_auto_dispose = true};
  env->bindings = (strmap_t)allocator_create(allocator, &g_strmap_type, &si);
  return env;
}

void comptime_env_dispose(comptime_env_t self) {
  if (!self) return;
  strmap_clear(self->bindings);
  allocator_free(self->allocator, &self->bindings);
  allocator_free(self->allocator, &self);
}

void comptime_env_bind(comptime_env_t self, const char *name,
                        comptime_value_t value) {
  strmap_insert(self->bindings, name, value);
}

comptime_value_t comptime_env_lookup(comptime_env_t self, const char *name) {
  for (comptime_env_t env = self; env; env = env->parent) {
    comptime_value_t v = (comptime_value_t)strmap_find(env->bindings, name);
    if (v) return v;
  }
  return NULL;
}

bool comptime_env_update(comptime_env_t self, const char *name,
                          comptime_value_t value) {
  for (comptime_env_t env = self; env; env = env->parent) {
    if (strmap_find(env->bindings, name)) {
      strmap_insert(env->bindings, name, value);
      return true;
    }
  }
  return false;
}
