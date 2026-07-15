#include "engine/comptime_alloc.h"
#include "core/allocator.h"
#include "core/string.h"
#include "core/strmap.h"
#include <stdio.h>
#include <stdlib.h>

/* ===== helpers ===== */

/* Stack-based key for strmap operations (strmap copies the key internally). */
#define _ADDR_KEY(buf, addr) snprintf((buf), sizeof(buf), "%llu", (unsigned long long)(addr))

static int _depth_from_ptr(void *p) {
  return (int)(intptr_t)p;
}

static void *_depth_to_ptr(int depth) {
  return (void *)(intptr_t)depth;
}

static char *_key_copy(allocator_t allocator, const char *src) {
  string_init_t si = {.str = src};
  string_t s = (string_t)allocator_create(allocator, &g_string_type, &si);
  return (char *)string_get(s);
}

/* ===== lifecycle ===== */

comptime_allocator_t comptime_allocator_create(allocator_t allocator) {
  comptime_allocator_t self =
      (comptime_allocator_t)allocator_alloc(allocator, sizeof(struct comptime_alloc));
  if (!self) return NULL;
  self->allocator = allocator;
  self->next_addr = 1;
  self->scope_depth = 0;
  strmap_init_t si = {.value_auto_dispose = false};
  self->allocations = (strmap_t)allocator_create(allocator, &g_strmap_type, &si);
  strmap_init_t li = {.value_auto_dispose = false};
  self->lifetimes = (strmap_t)allocator_create(allocator, &g_strmap_type, &li);
  return self;
}

void comptime_allocator_dispose(comptime_allocator_t self) {
  if (!self) return;
  allocator_t a = self->allocator;
  strmap_clear(self->allocations);
  allocator_free(a, &self->allocations);
  strmap_clear(self->lifetimes);
  allocator_free(a, &self->lifetimes);
  allocator_free(a, &self);
}

/* ===== allocate / read / write / free ===== */

uint64_t comptime_alloc_allocate(comptime_allocator_t self,
                                  comptime_value_t value, int scope_depth) {
  uint64_t addr = self->next_addr++;
  char key[21];
  _ADDR_KEY(key, addr);
  strmap_insert(self->allocations, key, value);
  strmap_insert(self->lifetimes, key, _depth_to_ptr(scope_depth));
  return addr;
}

comptime_value_t comptime_alloc_read(comptime_allocator_t self, uint64_t addr) {
  if (addr == 0) return NULL;
  char key[21];
  _ADDR_KEY(key, addr);
  return (comptime_value_t)strmap_find(self->allocations, key);
}

bool comptime_alloc_write(comptime_allocator_t self, uint64_t addr,
                           comptime_value_t value) {
  if (addr == 0) return false;
  char key[21];
  _ADDR_KEY(key, addr);
  if (!strmap_find(self->allocations, key)) return false;
  strmap_insert(self->allocations, key, value);
  return true;
}

void comptime_alloc_free(comptime_allocator_t self, uint64_t addr) {
  if (addr == 0) return;
  char key[21];
  _ADDR_KEY(key, addr);
  strmap_remove(self->allocations, key);
  strmap_remove(self->lifetimes, key);
}

/* ===== scope management ===== */

void comptime_alloc_enter_scope(comptime_allocator_t self) {
  self->scope_depth++;
}

void comptime_alloc_leave_scope(comptime_allocator_t self) {
  if (self->scope_depth <= 0) return;
  int leaving_depth = self->scope_depth;
  self->scope_depth--;

  /* Collect addresses to free at this depth.
     We copy keys first because strmap_remove invalidates iteration. */
  vec_t to_free = NULL;
  vec_init_t vi = {.auto_dispose = false};
  to_free = (vec_t)allocator_create(self->allocator, &g_vec_type, &vi);

  strmap_iter_t it = strmap_iter_first(self->lifetimes);
  const char *akey = NULL;
  while ((akey = strmap_iter_next(&it)) != NULL) {
    int depth = _depth_from_ptr(strmap_find(self->lifetimes, akey));
    if (depth >= leaving_depth) {
      char *copy = _key_copy(self->allocator, akey);
      vec_push(to_free, copy);
    }
  }

  for (size_t i = 0; i < vec_get_size(to_free); i++) {
    const char *k = (const char *)vec_get(to_free, i);
    strmap_remove(self->allocations, k);
    strmap_remove(self->lifetimes, k);
  }

  allocator_free(self->allocator, &to_free);
}
