#include "core/slotmap.h"
#include <string.h>

static void _slotmap_init(void *self, allocator_t allocator, void *arg) {
  (void)arg;
  slotmap_t *sm = (slotmap_t *)self;
  sm->allocator = allocator;
  sm->entries = NULL;
  sm->capacity = 0;
  sm->count = 0;
  sm->free_list = NULL;
  sm->free_count = 0;
}

static void _slotmap_dispose(void *self, allocator_t allocator) {
  slotmap_t *sm = (slotmap_t *)self;
  allocator_free(allocator, (void **)&sm->entries);
  allocator_free(allocator, (void **)&sm->free_list);
}

class_t g_slotmap_class = {
    .size = sizeof(slotmap_t),
    .name = "cubec.core.slotmap",
    .init = (class_init_fn_t)_slotmap_init,
    .dispose = (class_dispose_fn_t)_slotmap_dispose,
};

slotmap_t *slotmap_create(allocator_t allocator) {
  return (slotmap_t *)allocator_create(allocator, &g_slotmap_class, NULL);
}

void slotmap_dispose(slotmap_t *self) {
  if (!self) return;
  allocator_free(self->allocator, &self);
}

static void _slotmap_grow(slotmap_t *self, uint64_t needed) {
  if (needed <= self->capacity) return;

  uint64_t new_cap = self->capacity ? self->capacity : 8;
  while (new_cap < needed) new_cap *= 2;

  size_t old_bytes = self->capacity * sizeof(slot_entry_t);
  size_t new_bytes = new_cap * sizeof(slot_entry_t);

  slot_entry_t *new_entries =
      (slot_entry_t *)allocator_alloc(self->allocator, new_bytes);
  if (self->entries) {
    memcpy(new_entries, self->entries, old_bytes);
  }
  memset((char *)new_entries + old_bytes, 0, new_bytes - old_bytes);
  allocator_free(self->allocator, (void **)&self->entries);
  self->entries = new_entries;

  /* grow free_list to match new capacity */
  size_t fl_new_bytes = new_cap * sizeof(uint32_t);
  uint32_t *new_fl = (uint32_t *)allocator_alloc(self->allocator, fl_new_bytes);
  if (self->free_list) {
    memcpy(new_fl, self->free_list, self->free_count * sizeof(uint32_t));
  }
  allocator_free(self->allocator, (void **)&self->free_list);
  self->free_list = new_fl;

  self->capacity = new_cap;
}

slot_id_t slotmap_insert(slotmap_t *self, void *ptr) {
  if (self->free_count > 0) {
    /* reuse a free slot */
    uint64_t idx = self->free_list[--self->free_count];
    slot_entry_t *e = &self->entries[idx];
    e->ptr = ptr;
    return slot_id_make(idx, e->version);
  }

  /* append new slot */
  uint64_t idx = self->count;
  _slotmap_grow(self, idx + 1);
  slot_entry_t *e = &self->entries[idx];
  e->version = 0;
  e->ptr = ptr;
  self->count++;
  return slot_id_make(idx, 0);
}

void *slotmap_get(slotmap_t *self, slot_id_t id) {
  uint64_t idx = slot_id_index(id);
  uint16_t ver = slot_id_version(id);
  if (idx >= self->count) return NULL;
  slot_entry_t *e = &self->entries[idx];
  if (e->version != ver) return NULL;
  return e->ptr;
}

bool slotmap_remove(slotmap_t *self, slot_id_t id) {
  uint64_t idx = slot_id_index(id);
  uint16_t ver = slot_id_version(id);
  if (idx >= self->count) return false;
  slot_entry_t *e = &self->entries[idx];
  if (e->version != ver) return false;
  if (!e->ptr && ver != 0) return false; /* already free */

  e->ptr = NULL;
  e->version = (uint16_t)(ver + 1); /* wrap at 65535 → 0 is intentional */

  /* push to free list — grow if needed */
  if (self->free_count >= self->capacity) {
    uint64_t new_cap = self->capacity ? self->capacity * 2 : 8;
    size_t new_bytes = new_cap * sizeof(uint32_t);
    uint32_t *new_fl =
        (uint32_t *)allocator_alloc(self->allocator, new_bytes);
    if (self->free_list) {
      memcpy(new_fl, self->free_list, self->free_count * sizeof(uint32_t));
    }
    allocator_free(self->allocator, (void **)&self->free_list);
    self->free_list = new_fl;
  }
  self->free_list[self->free_count++] = (uint32_t)idx;
  return true;
}

uint64_t slotmap_compact(slotmap_t *self) {
  if (self->count == 0) return 0;

  uint64_t trimmed = 0;

  /* Walk backwards: trim consecutive trailing free slots */
  while (self->count > 0) {
    slot_entry_t *e = &self->entries[self->count - 1];
    if (e->ptr != NULL) break;
    /* Remove this index from free_list */
    uint32_t target = (uint32_t)(self->count - 1);
    for (uint64_t i = 0; i < self->free_count; i++) {
      if (self->free_list[i] == target) {
        self->free_list[i] = self->free_list[--self->free_count];
        break;
      }
    }
    self->count--;
    trimmed++;
  }

  /* Shrink entries allocation if count < capacity/4 */
  if (self->count < self->capacity / 4 && self->capacity > 8) {
    uint64_t new_cap = self->capacity / 2;
    if (new_cap < 8) new_cap = 8;

    size_t new_bytes = new_cap * sizeof(slot_entry_t);
    slot_entry_t *new_entries =
        (slot_entry_t *)allocator_alloc(self->allocator, new_bytes);
    memcpy(new_entries, self->entries, self->count * sizeof(slot_entry_t));
    allocator_free(self->allocator, (void **)&self->entries);
    self->entries = new_entries;

    size_t fl_bytes = new_cap * sizeof(uint32_t);
    uint32_t *new_fl = (uint32_t *)allocator_alloc(self->allocator, fl_bytes);
    memcpy(new_fl, self->free_list, self->free_count * sizeof(uint32_t));
    allocator_free(self->allocator, (void **)&self->free_list);
    self->free_list = new_fl;

    self->capacity = new_cap;
  }

  return trimmed;
}
