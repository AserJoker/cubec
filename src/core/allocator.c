#include "core/allocator.h"
#include <stddef.h>
#include <stdint.h>
struct _cubec_allocator_t {
  void *(*alloc)(size_t len);
  void (*free)(void *data);
};
struct _cubec_control_block_t {
  cubec_dispose_fn_t dispose;
  uint8_t data[0];
};

typedef struct _cubec_control_block_t *cubec_control_block_t;

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
  allocator->free(allocator);
}
void *cubec_allocator_alloc(cubec_allocator_t self, size_t len,
                            void (*dispose)(void *self,
                                            cubec_allocator_t allocator)) {
  if (!len) {
    return NULL;
  }
  cubec_control_block_t control_block =
      (cubec_control_block_t)self->alloc(sizeof(cubec_dispose_fn_t) + len);
  control_block->dispose = dispose;
  return &control_block->data;
}
void cubec_allocator_free(cubec_allocator_t self, void *data) {
  if (data == NULL) {
    return;
  }
  cubec_control_block_t block =
      (cubec_control_block_t)(((uint8_t *)data) - sizeof(cubec_dispose_fn_t));
  if (block->dispose != NULL) {
    block->dispose(block->data, self);
  }
  self->free(block);
}