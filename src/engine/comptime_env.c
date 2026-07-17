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
  /* bindings: name → addr (uint64_t stored as void*), NOT auto-disposed
     because values live in comptime_alloc, not in env */
  strmap_init_t si = {.value_auto_dispose = false};
  env->bindings = (strmap_t)allocator_create(allocator, &g_strmap_type, &si);
  /* temporaries: auto-disposed values created during expression evaluation */
  vec_init_t vi = {.auto_dispose = true};
  env->temporaries = (vec_t)allocator_create(allocator, &g_vec_type, &vi);
  return env;
}

void comptime_env_dispose(comptime_env_t self) {
  if (!self) return;
  vec_resize(self->temporaries, 0);
  allocator_free(self->allocator, &self->temporaries);
  /* No auto_dispose on bindings — values are owned by comptime_alloc */
  strmap_clear(self->bindings);
  allocator_free(self->allocator, &self->bindings);
  allocator_free(self->allocator, &self);
}

/* --- low-level addr API --- */

void comptime_env_bind(comptime_env_t self, const char *name, uint64_t addr) {
  strmap_insert(self->bindings, name, (void *)(uintptr_t)addr);
}

uint64_t comptime_env_lookup_addr(comptime_env_t self, const char *name) {
  for (comptime_env_t env = self; env; env = env->parent) {
    void *found = strmap_find(env->bindings, name);
    if (found) return (uint64_t)(uintptr_t)found;
  }
  return 0;
}

bool comptime_env_update_addr(comptime_env_t self, const char *name,
                              uint64_t addr) {
  for (comptime_env_t env = self; env; env = env->parent) {
    if (strmap_find(env->bindings, name)) {
      strmap_insert(env->bindings, name, (void *)(uintptr_t)addr);
      return true;
    }
  }
  return false;
}

/* --- convenience value API (alloc + bind/lookup/update) --- */

void comptime_env_bind_value(comptime_env_t self, comptime_allocator_t valloc,
                              const char *name, comptime_value_t value) {
  uint64_t addr = comptime_alloc_allocate(valloc, value, valloc->scope_depth);
  comptime_env_bind(self, name, addr);
}

comptime_value_t comptime_env_lookup_value(comptime_env_t self,
                                           comptime_allocator_t valloc,
                                           const char *name) {
  uint64_t addr = comptime_env_lookup_addr(self, name);
  if (addr == 0) return NULL;
  return comptime_alloc_read(valloc, addr);
}

bool comptime_env_update_value(comptime_env_t self, comptime_allocator_t valloc,
                                const char *name, comptime_value_t value) {
  uint64_t addr = comptime_env_lookup_addr(self, name);
  if (addr == 0) return false;
  return comptime_alloc_write(valloc, addr, value);
}

comptime_value_t comptime_env_track_temp(comptime_env_t self, comptime_value_t value) {
  if (value && self) vec_push(self->temporaries, value);
  return value;
}
