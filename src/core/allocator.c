#include "core/allocator.h"
#include "core/error.h"
#include "core/type.h"
#include <inttypes.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
};
allocator_t create_allocator(alloc_fn_t alloc_fn, free_fn_t free_fn) {
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
  return self;
}
void delete_allocator(allocator_t self) {
  if (self == NULL) {
    return;
  }
  while (self->chunks) {
    alloc_chunk_t chunk = self->chunks;
    fprintf(stderr, "memory leak: size = %" PRIuPTR ", pointer = %p",
            chunk->size, &chunk->data);
    if (chunk->type) {
      fprintf(stderr, ", type=%s", chunk->type->name);
    }
    fprintf(stderr, "\n");
    self->chunks = self->chunks->next;
    chunk->last = NULL;
    chunk->next = NULL;
    self->free_fn(chunk);
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
  return &chunk->data[0];
}

void *allocator_create(allocator_t self, type_t *type, void *arg) {
  void *data = allocator_alloc(self, type->size);
  alloc_chunk_t chunk = value_get_chunk(data);
  chunk->type = type;
  if (type->init) {
    TRY_VOID(NULL, type->init(data, self, arg));
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
      alloc_chunk_t chunk = value_get_chunk(data);
      chunk->type = type;
      TRY_VOID(NULL, type->clone(data, allocator, another));
      return data;
    } else {
      THROW(NULL, "%s does not support cloning", type->name);
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
      alloc_chunk_t chunk = value_get_chunk(data);
      chunk->type = type;
      TRY_VOID(NULL, type->move(data, allocator, another));
      return data;
    } else {
      THROW(NULL, "%s is not movable", type->name);
    }
  } else {
    alloc_chunk_t chunk = value_get_chunk(another);
    void *data = allocator_alloc(allocator, chunk->size);
    memcpy(data, another, chunk->size);
    memset(another,0,chunk->size);
    return data;
  }
}
