#include "core/allocator.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
typedef struct _control_block_t *control_block_t;
struct _allocator_t {
  void *(*alloc)(size_t len);
  void (*free)(void *data);
  control_block_t list;
};

struct _control_block_t {
  dispose_fn_t dispose;
  const char *filename;
  size_t line;
  size_t size;
  control_block_t next;
};

allocator_t create_allocator(allocator_initialize_t *initialize) {
  void *(*alloc_fn)(size_t len) = malloc;
  void (*free_fn)(void *data) = free;
  if (initialize != NULL) {
    alloc_fn = initialize->alloc;
    free_fn = initialize->free;
  }
  allocator_t allocator = alloc_fn(sizeof(struct _allocator_t));
  allocator->alloc = alloc_fn;
  allocator->free = free_fn;
  allocator->list = NULL;
  return allocator;
}
void delete_allocator(allocator_t allocator) {
  while (allocator->list != NULL) {
    control_block_t node = allocator->list;
    allocator->list = node->next;
    fprintf(stderr, "Memory leak: 0x%" PRIxPTR ", %s:%" PRIuPTR "\n",
            (intptr_t)&node[1], node->filename, node->line);
  }
  allocator->free(allocator);
}

void *allocator_alloc_debug(allocator_t self, size_t len, dispose_fn_t dispose,
                            const char *filename, size_t line) {
  if (!len) {
    return NULL;
  }
  control_block_t control_block =
      (control_block_t)self->alloc(sizeof(struct _control_block_t) + len);
  control_block->dispose = dispose;
  control_block->size = len;
  control_block->filename = filename;
  control_block->line = line;
  control_block->next = NULL;
  if (!self->list) {
    self->list = control_block;
  } else {
    control_block->next = self->list;
    self->list = control_block;
  }
  return &control_block[1];
}
void *_allocator_alloc(allocator_t self, size_t len,
                       void (*dispose)(void *self, allocator_t allocator)) {
  if (!len) {
    return NULL;
  }
  control_block_t control_block =
      (control_block_t)self->alloc(sizeof(struct _control_block_t) + len);
  control_block->dispose = dispose;
  control_block->size = len;
  control_block->filename = "";
  control_block->line = 0;
  control_block->next = NULL;
  if (!self->list) {
    self->list = control_block;
  } else {
    control_block->next = self->list;
    self->list = control_block;
  }
  return &control_block[1];
}
void allocator_free(allocator_t self, void *data) {
  if (data == NULL) {
    return;
  }
  control_block_t block =
      (control_block_t)((uint8_t *)data - sizeof(struct _control_block_t));
  if (block->dispose != NULL) {
    block->dispose(data, self);
  }
  if (self->list == block) {
    self->list = block->next;
  } else {
    control_block_t node = self->list;
    while (node->next != block) {
      node = node->next;
    }
    node->next = block->next;
  }
  self->free(block);
}