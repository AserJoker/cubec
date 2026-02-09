#include "core/array.h"
#include "core/allocator.h"
struct _cubec_array_t {
  void **data;
  size_t size;
  size_t capacity;
  bool autofree;
};
static void cubec_array_dispose(cubec_array_t self,
                                cubec_allocator_t allocator) {
  cubec_array_clear(self, allocator);
}
cubec_array_t cubec_create_array(cubec_allocator_t allocator,
                                 cubec_array_initialize_t *initialize) {
  cubec_array_t array =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_array_t),
                            (cubec_dispose_fn_t)cubec_array_dispose);
  array->autofree = false;
  array->capacity = 0;
  array->size = 0;
  array->data = NULL;
  if (initialize) {
    array->autofree = initialize->autofree;
    array->capacity = initialize->capacity;
    if (array->capacity) {
      array->data = cubec_allocator_alloc(
          allocator, sizeof(void *) * array->capacity, NULL);
      for (size_t idx = 0; idx < array->capacity; idx++) {
        array->data[idx] = NULL;
      }
    }
  }
  return array;
}
size_t cubec_array_get_size(cubec_array_t self) { return self->size; }
size_t cubec_array_get_capacity(cubec_array_t self) { return self->capacity; }
cubec_array_t cubec_array_shrink_to_fit(cubec_array_t self,
                                        cubec_allocator_t allocator) {
  if (self->size < self->capacity) {
    void **data =
        cubec_allocator_alloc(allocator, sizeof(void *) * self->size, NULL);
    for (size_t idx = 0; idx < self->size; idx++) {
      data[idx] = self->data[idx];
    }
    cubec_allocator_free(allocator, self->data);
    self->data = data;
    self->capacity = self->size;
  }
  return self;
}
cubec_array_t cubec_array_resize(cubec_array_t self,
                                 cubec_allocator_t allocator, size_t size) {
  void **data = cubec_allocator_alloc(allocator, sizeof(void *) * size, NULL);
  for (size_t idx = 0; idx < self->size; idx++) {
    if (idx >= size) {
      if (self->autofree) {
        cubec_allocator_free(allocator, self->data[idx]);
      }
    } else {
      data[idx] = self->data[idx];
    }
  }
  for (size_t idx = self->size; idx < size; idx++) {
    data[idx] = NULL;
  }
  cubec_allocator_free(allocator, self->data);
  self->data = data;
  self->capacity = size;
  if (self->size > self->capacity) {
    self->size = self->capacity;
  }
  return self;
}
void *cubec_array_get_index(cubec_array_t self, size_t index) {
  if (index >= self->size) {
    return NULL;
  }
  return self->data[index];
}
void cubec_array_set_index(cubec_array_t self, cubec_allocator_t allocator,
                           size_t index, void *data) {
  if (index >= self->size) {
    return;
  }
  if (self->autofree && self->data[index] != data) {
    cubec_allocator_free(allocator, self->data[index]);
  }
  self->data[index] = data;
}
void cubec_array_push(cubec_array_t self, cubec_allocator_t allocator,
                      void *data) {
  if (self->size >= self->capacity) {
    cubec_array_resize(self, allocator,
                       self->capacity == 0 ? 1 : self->capacity * 2);
  }
  self->data[self->size] = data;
  self->size++;
}
void cubec_array_pop(cubec_array_t self, cubec_allocator_t allocator) {
  if (self->size) {
    if (self->autofree) {
      cubec_allocator_free(allocator, self->data[self->size - 1]);
    }
    self->data[self->size - 1] = NULL;
    self->size--;
  }
}
void *cubec_array_back(cubec_array_t self, cubec_allocator_t allocator) {
  if (self->size) {
    return self->data[self->size - 1];
  }
  return NULL;
}
void cubec_array_swap(cubec_array_t self, size_t origin, size_t target) {
  if (origin < self->size && target < self->size) {
    void *tmp = self->data[origin];
    self->data[origin] = self->data[target];
    self->data[target] = tmp;
  }
}

void cubec_array_clear(cubec_array_t self, cubec_allocator_t allocator) {
  cubec_array_resize(self, allocator, 0);
}