#include "core/array.h"
#include "core/allocator.h"
#include "core/compare.h"
#include <stdint.h>
struct _cubec_array_t {
  void **data;
  size_t size;
  size_t capacity;
  bool autofree;
  cubec_allocator_t allocator;
  cubec_compare_fn_t compare;
};
static void cubec_array_dispose(cubec_array_t self,
                                cubec_allocator_t allocator) {
  cubec_array_clear(self);
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
  array->allocator = allocator;
  array->compare = NULL;
  if (initialize) {
    array->autofree = initialize->autofree;
    array->capacity = initialize->capacity;
    array->compare = initialize->compare;
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
size_t cubec_array_get_size(const cubec_array_t self) { return self->size; }
size_t cubec_array_get_capacity(const cubec_array_t self) {
  return self->capacity;
}
cubec_array_t cubec_array_shrink_to_fit(cubec_array_t self) {
  if (self->size < self->capacity) {
    void **data = cubec_allocator_alloc(self->allocator,
                                        sizeof(void *) * self->size, NULL);
    for (size_t idx = 0; idx < self->size; idx++) {
      data[idx] = self->data[idx];
    }
    cubec_allocator_free(self->allocator, self->data);
    self->data = data;
    self->capacity = self->size;
  }
  return self;
}
cubec_array_t cubec_array_resize(cubec_array_t self, size_t size) {
  void **data =
      cubec_allocator_alloc(self->allocator, sizeof(void *) * size, NULL);
  for (size_t idx = 0; idx < self->size; idx++) {
    if (idx >= size) {
      if (self->autofree) {
        cubec_allocator_free(self->allocator, self->data[idx]);
      }
    } else {
      data[idx] = self->data[idx];
    }
  }
  for (size_t idx = self->size; idx < size; idx++) {
    data[idx] = NULL;
  }
  cubec_allocator_free(self->allocator, self->data);
  self->data = data;
  self->capacity = size;
  if (self->size > self->capacity) {
    self->size = self->capacity;
  }
  return self;
}
void *cubec_array_get(const cubec_array_t self, size_t index) {
  if (index >= self->size) {
    return NULL;
  }
  return self->data[index];
}
void cubec_array_del(cubec_array_t self, size_t index) {
  if (index >= self->size) {
    return;
  }
  if (self->autofree) {
    cubec_allocator_free(self->allocator, self->data[index]);
  }
  while (index < self->size - 1) {
    self->data[index] = self->data[index + 1];
    index++;
  }
  self->size--;
  self->data[self->size] = NULL;
}

void *cubec_array_move(cubec_array_t self, size_t index) {
  if (index >= self->size) {
    return NULL;
  }
  void *data = self->data[index];
  while (index < self->size - 1) {
    self->data[index] = self->data[index + 1];
    index++;
  }
  self->size--;
  self->data[self->size] = NULL;
  return data;
}
void *cubec_array_replace(cubec_array_t self, size_t index, void *data) {
  if (index >= self->size) {
    return NULL;
  }
  void *current = self->data[index];
  self->data[index] = data;
  return current;
}
void cubec_array_set(cubec_array_t self, size_t index, void *data) {
  if (index >= self->size) {
    return;
  }
  if (self->autofree && self->data[index] != data) {
    cubec_allocator_free(self->allocator, self->data[index]);
  }
  self->data[index] = data;
}

void cubec_array_insert(cubec_array_t self, size_t index, void *data) {
  if (self->size + 1 >= self->capacity) {
    cubec_array_resize(self, self->size + 1);
  }
  for (size_t idx = index; idx < self->size; idx++) {
    self->data[idx + 1] = self->data[idx];
  }
  self->data[index] = data;
  self->size++;
}

void cubec_array_push(cubec_array_t self, void *data) {
  if (self->size >= self->capacity) {
    cubec_array_resize(self, self->capacity == 0 ? 1 : self->capacity * 2);
  }
  self->data[self->size] = data;
  self->size++;
}
void cubec_array_pop(cubec_array_t self) {
  if (self->size) {
    if (self->autofree) {
      cubec_allocator_free(self->allocator, self->data[self->size - 1]);
    }
    self->data[self->size - 1] = NULL;
    self->size--;
  }
}
void *cubec_array_back(cubec_array_t self) {
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

void cubec_array_clear(cubec_array_t self) { cubec_array_resize(self, 0); }

static void cubec_array_qsort(void **begin, size_t size,
                              cubec_compare_fn_t compare, void *cmp_arg) {
  if (size <= 1) {
    return;
  }
  void *pivot = begin[size - 1];
  size_t i = 0;
  for (size_t j = 0; j < size - 1; j++) {
    if (compare(begin[j], pivot, cmp_arg) <= 0) {
      void *tmp = begin[i];
      begin[i] = begin[j];
      begin[j] = tmp;
      i++;
    }
  }
  void *temp = begin[i];
  begin[i] = pivot;
  begin[size - 1] = temp;
  cubec_array_qsort(begin, i, compare, cmp_arg);
  cubec_array_qsort(begin + i + 1, size - i - 1, compare, cmp_arg);
}

void cubec_array_sort(cubec_array_t self, void *cmp_arg) {
  if (self->compare) {
    cubec_array_qsort(self->data, self->size, self->compare, cmp_arg);
  }
}
cubec_array_t cubec_array_clone(cubec_allocator_t allocator,
                                const cubec_array_t src) {
  cubec_array_initialize_t initialize = {
      .autofree = src->autofree,
      .capacity = src->capacity,
      .compare = src->compare,
  };
  cubec_array_t array = cubec_create_array(allocator, &initialize);
  cubec_array_resize(array, src->size);
  for (size_t idx = 0; idx < src->size; idx++) {
    array->data[idx] = src->data[idx];
  }
  return array;
}
size_t cubec_array_find_index(cubec_array_t self, const void *value,
                              void *cmp_arg) {
  for (size_t idx = 0; idx < self->size; idx++) {
    if (self->compare) {
      if (self->compare(self->data[idx], value, cmp_arg) == 0) {
        return idx;
      }
    } else if (self->data[idx] == value) {
      return idx;
    }
  }
  return (size_t)-1;
}
void *cubec_array_get_data(cubec_array_t self) { return self->data; }