#include "core/allocator.h"
#include "core/type.h"
#include <inttypes.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef RUNNING_ON_VALGRIND
#include <valgrind/valgrind.h>
#endif
typedef struct _alloc_chunk_t *alloc_chunk_t;
struct _alloc_chunk_t {
  alloc_chunk_t next;
  alloc_chunk_t last;
  size_t size;
  type_t *type;
  uint64_t id;
  uint8_t data[0];
};
struct _allocator_t {
  alloc_fn_t alloc_fn;
  free_fn_t free_fn;
  atomic_uint_fast64_t id;
  alloc_chunk_t chunks;
  size_t total_allocated;
  size_t peak_allocated;
  size_t alloc_count;
  size_t free_count;
  size_t max_memory; /* 0 = unlimited */
};
allocator_t create_allocator(alloc_fn_t alloc_fn, free_fn_t free_fn) {
  return create_allocator_with_limit(alloc_fn, free_fn, 0);
}

allocator_t create_allocator_with_limit(alloc_fn_t alloc_fn, free_fn_t free_fn,
                                        size_t max_memory) {
  if (alloc_fn == NULL) {
    alloc_fn = malloc;
  }
  if (free_fn == NULL) {
    free_fn = free;
  }
  allocator_t self = (allocator_t)alloc_fn(sizeof(struct _allocator_t));
  if (self == NULL) {
    abort();
  }
  self->alloc_fn = alloc_fn;
  self->free_fn = free_fn;
  self->id = 0;
  self->chunks = NULL;
  self->total_allocated = 0;
  self->peak_allocated = 0;
  self->alloc_count = 0;
  self->free_count = 0;
  self->max_memory = max_memory;
  return self;
}
void delete_allocator(allocator_t self) {
  if (self == NULL) {
    return;
  }
  /* Report unfreed chunks (leaks) */
  if (self->chunks != NULL) {
    size_t leak_count = 0;
    size_t leak_bytes = 0;
    alloc_chunk_t chunk = self->chunks;
    fprintf(stderr,
            "\n=== ALLOCATOR LEAK REPORT ===\n"
            "Total allocations: %zu, frees: %zu\n"
            "Peak memory: %zu bytes (%.2f MB)\n"
            "Still alive at destruction: ",
            self->alloc_count, self->free_count, self->peak_allocated,
            self->peak_allocated / (1024.0 * 1024.0));
    while (chunk != NULL) {
      leak_count++;
      leak_bytes += chunk->size;
      const char *type_name =
          chunk->type ? chunk->type->name : "<raw>";
      fprintf(stderr, "\n  LEAK #%zu: %zu bytes, type=%s, id=%" PRIu64,
              leak_count, chunk->size, type_name, chunk->id);
      chunk = chunk->next;
    }
    fprintf(stderr,
            "\nLeaked: %zu chunks, %zu bytes (%.2f MB)\n"
            "=== END LEAK REPORT ===\n\n",
            leak_count, leak_bytes, leak_bytes / (1024.0 * 1024.0));
  }
  self->free_fn(self);
}
static alloc_chunk_t value_get_chunk(void *data) {
  uint8_t *addr = (uint8_t *)data;
  size_t offset = offsetof(struct _alloc_chunk_t, data);
  alloc_chunk_t chunk = (alloc_chunk_t)(addr - offset);
  return chunk;
}
void *allocator_alloc(allocator_t self, size_t size) {
  if (size == 0) {
    return NULL;
  }
  /* Check memory budget */
  if (self->max_memory > 0 &&
      self->total_allocated + size > self->max_memory) {
    fprintf(stderr,
            "FATAL: allocator memory budget exceeded: %zu + %zu > %zu "
            "(%.2f MB limit), alloc_count=%zu, free_count=%zu\n",
            self->total_allocated, size, self->max_memory,
            self->max_memory / (1024.0 * 1024.0),
            self->alloc_count, self->free_count);
    exit(99);
  }
  alloc_chunk_t chunk = self->alloc_fn(sizeof(struct _alloc_chunk_t) + size);
  if (chunk == NULL) {
    abort();
  }
  chunk->size = size;
  chunk->next = self->chunks;
  chunk->type = NULL;
  if (chunk->next) {
    chunk->next->last = chunk;
  }
  chunk->last = NULL;
  self->chunks = chunk;
  chunk->id = atomic_fetch_add(&self->id, 1);
  memset(&chunk->data[0], 0, size);
  /* Update stats */
  self->total_allocated += size;
  self->alloc_count++;
  if (self->total_allocated > self->peak_allocated) {
    self->peak_allocated = self->total_allocated;
  }
  return &chunk->data[0];
}

void *allocator_create(allocator_t self, type_t *type, void *arg) {
  void *data = allocator_alloc(self, type->size);
  alloc_chunk_t chunk = value_get_chunk(data);
  chunk->type = type;
  if (type->init) {
    type->init(data, self, arg);
  }
  return data;
}
void _allocator_free_impl(allocator_t self, void **data) {
  if (data == NULL || *data == NULL) {
    return;
  }
  alloc_chunk_t chunk = value_get_chunk(*data);
  if (chunk->type && chunk->type->dispose) {
    chunk->type->dispose(*data, self);
  }
  if (chunk == self->chunks) {
    self->chunks = self->chunks->next;
  }
  if (chunk->last) {
    chunk->last->next = chunk->next;
  }
  if (chunk->next) {
    chunk->next->last = chunk->last;
  }
  /* Update stats */
  self->total_allocated -= chunk->size;
  self->free_count++;
  self->free_fn(chunk);
  *data = NULL;
}
type_t *value_get_type(void *value) {
  alloc_chunk_t chunk = value_get_chunk(value);
  return chunk->type;
}

uint64_t value_get_id(void *self) {
  alloc_chunk_t chunk = value_get_chunk(self);
  return chunk->id;
}

void *value_clone(allocator_t allocator, void *another) {
  if (!another) {
    return NULL;
  }
  type_t *type = value_get_type(another);
  if (type) {
    if (type->clone) {
      void *data = allocator_alloc(allocator, type->size);
      if (!data) {
        return NULL;
      }
      alloc_chunk_t chunk = value_get_chunk(data);
      chunk->type = type;
      type->clone(data, allocator, another);
      return data;
    } else {
      abort();
    }
  } else {
    alloc_chunk_t chunk = value_get_chunk(another);
    void *data = allocator_alloc(allocator, chunk->size);
    memcpy(data, another, chunk->size);
    return data;
  }
}
void *value_move(allocator_t allocator, void *another) {
  if (!another) {
    return NULL;
  }
  type_t *type = value_get_type(another);
  if (type) {
    if (type->move) {
      void *data = allocator_alloc(allocator, type->size);
      if (!data) {
        return NULL;
      }
      alloc_chunk_t chunk = value_get_chunk(data);
      chunk->type = type;
      type->move(data, allocator, another);
      return data;
    } else {
      abort();
    }
  } else {
    alloc_chunk_t chunk = value_get_chunk(another);
    void *data = allocator_alloc(allocator, chunk->size);
    memcpy(data, another, chunk->size);
    memset(another,0,chunk->size);
    return data;
  }
}

size_t allocator_get_total(allocator_t self) {
  return self ? self->total_allocated : 0;
}

size_t allocator_get_peak(allocator_t self) {
  return self ? self->peak_allocated : 0;
}

size_t allocator_get_alloc_count(allocator_t self) {
  return self ? self->alloc_count : 0;
}

size_t allocator_get_free_count(allocator_t self) {
  return self ? self->free_count : 0;
}
