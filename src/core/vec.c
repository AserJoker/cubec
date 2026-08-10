#include "core/vec.h"
#include "core/allocator.h"

struct _vec_t {
  allocator_t allocator;
  size_t size;
  size_t capacity;
  bool auto_dispose;
  void **data;
};

static void _vec_init(vec_t self, allocator_t allocator, vec_init_t *init) {
  if (init) {
    self->auto_dispose = init->auto_dispose;
  } else {
    self->auto_dispose = false;
  }
  self->capacity = 0;
  self->size = 0;
  self->data = NULL;
  self->allocator = allocator;
}
static void _vec_clone(vec_t self, allocator_t allocator, vec_t another) {
  self->auto_dispose = another->auto_dispose;
  self->capacity = another->capacity;
  self->size = another->size;
  self->data = allocator_alloc(allocator, sizeof(void *) * self->capacity);
  for (size_t idx = 0; idx < self->size; idx++) {
    self->data[idx] = alloc_clone(allocator, another->data[idx]);
  }
}
static void _vec_move(vec_t self, allocator_t allocator, vec_t another) {
  (void)allocator;
  self->auto_dispose = another->auto_dispose;
  self->capacity = another->capacity;
  self->size = another->size;
  self->data = another->data;
  another->data = NULL;
  another->size = 0;
  another->capacity = 0;
}
static void _vec_dispose(vec_t self, allocator_t allocator) {
  if (self->auto_dispose) {
    for (size_t idx = 0; idx < self->size; idx++) {
      allocator_free(allocator, &self->data[idx]);
    }
  }
  allocator_free(allocator, &self->data);
  self->size = 0;
  self->capacity = 0;
}

type_t g_vec_type = {
    .size = sizeof(struct _vec_t),
    .name = "cubec.core.vec",
    .init = (type_init_fn_t)_vec_init,
    .dispose = (type_dispose_fn_t)_vec_dispose,
    .clone = (type_clone_fn_t)_vec_clone,
    .move = (type_move_fn_t)_vec_move,
};

size_t vec_get_size(vec_t self) { return self->size; }
size_t vec_get_capacity(vec_t self) { return self->capacity; }
void **vec_get_data(vec_t self) { return self->data; }
void *vec_get(vec_t self, size_t idx) {
  if (idx >= self->size) {
    return NULL;
  }
  return self->data[idx];
}
size_t vec_set(vec_t self, size_t idx, void *data) {
  if (idx >= self->size) {
    return (size_t)-1;
  }
  if (self->auto_dispose) {
    allocator_free(self->allocator, &self->data[idx]);
  }
  self->data[idx] = data;
  return self->size;
}
size_t vec_resize(vec_t self, size_t size) {
  for (size_t idx = size; idx < self->size; idx++) {
    if (self->auto_dispose) {
      allocator_free(self->allocator, &self->data[idx]);
    }
    self->data[idx] = NULL;
  }
  if (size >= self->capacity) {
    if (self->capacity == 0) {
      self->capacity = 8;
    }
    while (self->capacity < size) {
      self->capacity *= 2;
    }
    void **data =
        allocator_alloc(self->allocator, sizeof(void *) * self->capacity);
    for (size_t idx = 0; idx < self->size; idx++) {
      data[idx] = self->data[idx];
    }
    allocator_free(self->allocator, &self->data);
    self->data = data;
  }
  self->size = size;
  return self->size;
}
size_t vec_push(vec_t self, void *data) {
  vec_resize(self, self->size + 1);
  self->data[self->size - 1] = data;
  return self->size;
}
size_t vec_pop(vec_t self) {
  if (!self->size) {
    return (size_t)-1;
  }
  return vec_resize(self, self->size - 1);
}
size_t vec_remove(vec_t self, size_t idx) {
  if (idx >= self->size) {
    return (size_t)-1;
  }
  void *item = self->data[idx];
  for (size_t i = idx; i < self->size - 1; i++) {
    self->data[i] = self->data[i + 1];
  }
  self->size--;
  self->data[self->size] = NULL;
  if (self->auto_dispose) {
    allocator_free(self->allocator, &item);
  }
  return self->size;
}
size_t vec_insert(vec_t self, size_t idx, void *data) {
  if (vec_resize(self, self->size + 1) == (size_t)-1) {
    return (size_t)-1;
  }
  for (size_t i = self->size - 1; i > idx; i--) {
    self->data[i] = self->data[i - 1];
  }
  self->data[idx] = data;
  return self->size;
}

/* ============================================================================
 *  Iterator
 * ============================================================================ */

vec_iter_t vec_iter_first(vec_t vec) {
  vec_iter_t iter = {
      .vec = vec,
      .idx = 0,
  };
  return iter;
}

void *vec_iter_next(vec_iter_t *iter) {
  if (iter->idx >= iter->vec->size) {
    return NULL;
  }
  return iter->vec->data[iter->idx++];
}

void *vec_iter_get(vec_iter_t *iter) {
  if (iter->idx >= iter->vec->size) {
    return NULL;
  }
  return iter->vec->data[iter->idx];
}

void *vec_iter_set(vec_iter_t *iter, void *data) {
  if (iter->idx >= iter->vec->size) {
    return NULL;
  }
  vec_t self = iter->vec;
  void *old_data = self->data[iter->idx];
  self->data[iter->idx] = data;
  return old_data;
}

void *vec_iter_remove(vec_iter_t *iter) {
  if (iter->idx >= iter->vec->size) {
    return NULL;
  }
  vec_t self = iter->vec;
  void *data = self->data[iter->idx];
  /* Shift subsequent elements left */
  for (size_t i = iter->idx; i < self->size - 1; i++) {
    self->data[i] = self->data[i + 1];
  }
  self->size--;
  self->data[self->size] = NULL;
  if (self->auto_dispose) {
    allocator_free(self->allocator, &data);
  }
  /* idx stays at the same position, now pointing to the next element */
  return data;
}