#include "core/array.h"
#include "core/allocator.h"
#include "core/compare.h"
#include <stdint.h>
struct _array_t {
  void **data;
  size_t size;
  size_t capacity;
  bool autofree;
  allocator_t allocator;
  compare_fn_t compare;
};
static void array_dispose(array_t self, allocator_t allocator) {
  array_clear(self);
}
array_t create_array(allocator_t allocator, array_initialize_t *initialize) {
  array_t array = allocator_alloc(allocator, sizeof(struct _array_t),
                                  (dispose_fn_t)array_dispose);
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
      array->data =
          allocator_alloc(allocator, sizeof(void *) * array->capacity, NULL);
      for (size_t idx = 0; idx < array->capacity; idx++) {
        array->data[idx] = NULL;
      }
    }
  }
  return array;
}
size_t array_get_size(const array_t self) { return self->size; }
size_t array_get_capacity(const array_t self) { return self->capacity; }
array_t array_shrink_to_fit(array_t self) {
  if (self->size < self->capacity) {
    void **data =
        allocator_alloc(self->allocator, sizeof(void *) * self->size, NULL);
    for (size_t idx = 0; idx < self->size; idx++) {
      data[idx] = self->data[idx];
    }
    allocator_free(self->allocator, self->data);
    self->data = data;
    self->capacity = self->size;
  }
  return self;
}
array_t array_resize(array_t self, size_t size) {
  void **data = allocator_alloc(self->allocator, sizeof(void *) * size, NULL);
  for (size_t idx = 0; idx < self->size; idx++) {
    if (idx >= size) {
      if (self->autofree) {
        allocator_free(self->allocator, self->data[idx]);
      }
    } else {
      data[idx] = self->data[idx];
    }
  }
  for (size_t idx = self->size; idx < size; idx++) {
    data[idx] = NULL;
  }
  allocator_free(self->allocator, self->data);
  self->data = data;
  self->capacity = size;
  if (self->size > self->capacity) {
    self->size = self->capacity;
  }
  return self;
}
void *array_get(const array_t self, size_t index) {
  if (index >= self->size) {
    return NULL;
  }
  return self->data[index];
}
void array_del(array_t self, size_t index) {
  if (index >= self->size) {
    return;
  }
  if (self->autofree) {
    allocator_free(self->allocator, self->data[index]);
  }
  while (index < self->size - 1) {
    self->data[index] = self->data[index + 1];
    index++;
  }
  self->size--;
  self->data[self->size] = NULL;
}

void *array_move(array_t self, size_t index) {
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
void *array_replace(array_t self, size_t index, void *data) {
  if (index >= self->size) {
    return NULL;
  }
  void *current = self->data[index];
  self->data[index] = data;
  return current;
}
void array_set(array_t self, size_t index, void *data) {
  if (index >= self->size) {
    return;
  }
  if (self->autofree && self->data[index] != data) {
    allocator_free(self->allocator, self->data[index]);
  }
  self->data[index] = data;
}

void array_insert(array_t self, size_t index, void *data) {
  if (self->size + 1 >= self->capacity) {
    array_resize(self, self->size + 1);
  }
  for (size_t idx = index; idx < self->size; idx++) {
    self->data[idx + 1] = self->data[idx];
  }
  self->data[index] = data;
  self->size++;
}

void array_push(array_t self, void *data) {
  if (self->size >= self->capacity) {
    array_resize(self, self->capacity == 0 ? 1 : self->capacity * 2);
  }
  self->data[self->size] = data;
  self->size++;
}
void array_pop(array_t self) {
  if (self->size) {
    if (self->autofree) {
      allocator_free(self->allocator, self->data[self->size - 1]);
    }
    self->data[self->size - 1] = NULL;
    self->size--;
  }
}
void *array_back(array_t self) {
  if (self->size) {
    return self->data[self->size - 1];
  }
  return NULL;
}
void array_swap(array_t self, size_t origin, size_t target) {
  if (origin < self->size && target < self->size) {
    void *tmp = self->data[origin];
    self->data[origin] = self->data[target];
    self->data[target] = tmp;
  }
}

void array_clear(array_t self) { array_resize(self, 0); }

static void array_qsort(void **begin, size_t size, compare_fn_t compare,
                        void *cmp_arg) {
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
  array_qsort(begin, i, compare, cmp_arg);
  array_qsort(begin + i + 1, size - i - 1, compare, cmp_arg);
}

void array_sort(array_t self, void *cmp_arg) {
  if (self->compare) {
    array_qsort(self->data, self->size, self->compare, cmp_arg);
  }
}
array_t array_clone(allocator_t allocator, const array_t src) {
  array_initialize_t initialize = {
      .autofree = src->autofree,
      .capacity = src->capacity,
      .compare = src->compare,
  };
  array_t array = create_array(allocator, &initialize);
  array_resize(array, src->size);
  for (size_t idx = 0; idx < src->size; idx++) {
    array->data[idx] = src->data[idx];
  }
  return array;
}
size_t array_find_index(array_t self, const void *value, void *cmp_arg) {
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
void *array_get_data(array_t self) { return self->data; }