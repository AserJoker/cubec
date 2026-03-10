#include "core/allocator.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
typedef struct _cubec_control_block_t *cubec_control_block_t;
struct _cubec_allocator_t {
  void *(*alloc)(size_t len);
  void (*free)(void *data);
  cubec_control_block_t list;
};

struct _cubec_control_block_t {
  cubec_dispose_fn_t dispose;
  const char *filename;
  size_t line;
  size_t size;
  cubec_control_block_t next;
};

cubec_allocator_t
cubec_create_allocator(cubec_allocator_initialize_t *initialize) {
  void *(*alloc_fn)(size_t len) = malloc;
  void (*free_fn)(void *data) = free;
  if (initialize != NULL) {
    alloc_fn = initialize->alloc;
    free_fn = initialize->free;
  }
  cubec_allocator_t allocator = alloc_fn(sizeof(struct _cubec_allocator_t));
  allocator->alloc = alloc_fn;
  allocator->free = free_fn;
  return allocator;
}
void cubec_delete_allocator(cubec_allocator_t allocator) {
  while (allocator->list != NULL) {
    cubec_control_block_t node = allocator->list;
    allocator->list = node->next;
    fprintf(stderr, "Memory leak: 0x%" PRIxPTR ", %s:%" PRIuPTR "\n",
            (intptr_t)&node[1], node->filename, node->line);
  }
  allocator->free(allocator);
}

void *cubec_allocator_alloc_debug(cubec_allocator_t self, size_t len,
                                  cubec_dispose_fn_t dispose,
                                  const char *filename, size_t line) {
  if (!len) {
    return NULL;
  }
  cubec_control_block_t control_block = (cubec_control_block_t)self->alloc(
      sizeof(struct _cubec_control_block_t) + len);
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
void *_cubec_allocator_alloc(cubec_allocator_t self, size_t len,
                             void (*dispose)(void *self,
                                             cubec_allocator_t allocator)) {
  if (!len) {
    return NULL;
  }
  cubec_control_block_t control_block = (cubec_control_block_t)self->alloc(
      sizeof(struct _cubec_control_block_t) + len);
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
void cubec_allocator_free(cubec_allocator_t self, void *data) {
  if (data == NULL) {
    return;
  }
  cubec_control_block_t block =
      (cubec_control_block_t)((uint8_t *)data -
                              sizeof(struct _cubec_control_block_t));
  if (block->dispose != NULL) {
    block->dispose(data, self);
  }
  if (self->list == block) {
    self->list = block->next;
  } else {
    cubec_control_block_t node = self->list;
    while (node->next != block) {
      node = node->next;
    }
    node->next = block->next;
  }
  self->free(block);
}