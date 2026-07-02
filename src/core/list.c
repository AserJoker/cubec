#include "core/list.h"
#include "core/allocator.h"
#include "core/error.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct _list_node_t list_node_t;
struct _list_node_t {
  list_node_t *prev;
  list_node_t *next;
  void *data;
};

struct _list_t {
  allocator_t allocator;
  list_node_t *head;
  list_node_t *tail;
  size_t size;
  bool auto_dispose;
};

static void _list_init(list_t self, allocator_t allocator, list_init_t *init) {
  if (init) {
    self->auto_dispose = init->auto_dispose;
  } else {
    self->auto_dispose = false;
  }
  self->head = NULL;
  self->tail = NULL;
  self->size = 0;
  self->allocator = allocator;
}

static void _list_clone(list_t self, allocator_t allocator, list_t another) {
  self->auto_dispose = another->auto_dispose;
  self->head = NULL;
  self->tail = NULL;
  self->size = 0;
  self->allocator = allocator;
  list_node_t *node = another->head;
  while (node != NULL) {
    void *cloned_data = value_clone(allocator, node->data);
    list_push(self, cloned_data);
    node = node->next;
  }
}

static void _list_move(list_t self, allocator_t allocator, list_t another) {
  self->auto_dispose = another->auto_dispose;
  self->head = another->head;
  self->tail = another->tail;
  self->size = another->size;
  self->allocator = allocator;
  another->head = NULL;
  another->tail = NULL;
  another->size = 0;
}

static void _list_dispose(list_t self, allocator_t allocator) {
  list_clear(self);
}

type_t g_list_type = {
    .size = sizeof(struct _list_t),
    .name = "cubec.core.list",
    .init = (type_init_fn_t)_list_init,
    .dispose = (type_dispose_fn_t)_list_dispose,
    .clone = (type_clone_fn_t)_list_clone,
    .move = (type_move_fn_t)_list_move,
};

static list_node_t *create_list_node(allocator_t allocator, void *data) {
  list_node_t *node =
      (list_node_t *)allocator_alloc(allocator, sizeof(list_node_t));
  node->prev = NULL;
  node->next = NULL;
  node->data = data;
  return node;
}

size_t list_get_size(list_t self) { return self->size; }

void **list_get_data(list_t self) {
  void **data = allocator_alloc(self->allocator, sizeof(void *) * self->size);
  list_node_t *node = self->head;
  for (size_t idx = 0; idx < self->size; idx++) {
    data[idx] = node->data;
    node = node->next;
  }
  return data;
}

void *list_get_first(list_t self) {
  if (self->head == NULL) {
    THROW(NULL, "RangeError: list is empty");
  }
  return self->head->data;
}

void *list_get_last(list_t self) {
  if (self->tail == NULL) {
    THROW(NULL, "RangeError: list is empty");
  }
  return self->tail->data;
}

void *list_get(list_t self, size_t idx) {
  if (idx >= self->size) {
    THROW(NULL, "RangeError: index %" PRIuPTR " out of list length %" PRIuPTR,
          idx, self->size);
  }
  list_node_t *node;
  if (idx < self->size / 2) {
    node = self->head;
    for (size_t i = 0; i < idx; i++) {
      node = node->next;
    }
  } else {
    node = self->tail;
    for (size_t i = self->size - 1; i > idx; i--) {
      node = node->prev;
    }
  }
  return node->data;
}

size_t list_set(list_t self, size_t idx, void *data) {
  if (idx >= self->size) {
    THROW((size_t)-1,
          "RangeError: index %" PRIuPTR " out of list length %" PRIuPTR, idx,
          self->size);
  }
  list_node_t *node;
  if (idx < self->size / 2) {
    node = self->head;
    for (size_t i = 0; i < idx; i++) {
      node = node->next;
    }
  } else {
    node = self->tail;
    for (size_t i = self->size - 1; i > idx; i--) {
      node = node->prev;
    }
  }
  if (self->auto_dispose) {
    allocator_free(self->allocator, node->data);
  }
  node->data = data;
  return self->size;
}

size_t list_push(list_t self, void *data) {
  list_node_t *node = create_list_node(self->allocator, data);
  if (self->tail == NULL) {
    self->head = node;
    self->tail = node;
  } else {
    node->prev = self->tail;
    self->tail->next = node;
    self->tail = node;
  }
  self->size++;
  return self->size;
}

void *list_pop(list_t self) {
  if (self->tail == NULL) {
    THROW(NULL, "RangeError: list is empty");
  }
  list_node_t *node = self->tail;
  void *data = node->data;
  self->tail = node->prev;
  if (self->tail != NULL) {
    self->tail->next = NULL;
  } else {
    self->head = NULL;
  }
  if (self->auto_dispose) {
    allocator_free(self->allocator, data);
  }
  allocator_free(self->allocator, node);
  self->size--;
  return data;
}

size_t list_unshift(list_t self, void *data) {
  list_node_t *node = create_list_node(self->allocator, data);
  if (self->head == NULL) {
    self->head = node;
    self->tail = node;
  } else {
    node->next = self->head;
    self->head->prev = node;
    self->head = node;
  }
  self->size++;
  return self->size;
}

void *list_shift(list_t self) {
  if (self->head == NULL) {
    THROW(NULL, "RangeError: list is empty");
  }
  list_node_t *node = self->head;
  void *data = node->data;
  self->head = node->next;
  if (self->head != NULL) {
    self->head->prev = NULL;
  } else {
    self->tail = NULL;
  }
  if (self->auto_dispose) {
    allocator_free(self->allocator, data);
  }
  allocator_free(self->allocator, node);
  self->size--;
  return data;
}

size_t list_insert(list_t self, size_t idx, void *data) {
  if (idx > self->size) {
    THROW((size_t)-1,
          "RangeError: index %" PRIuPTR " out of list length %" PRIuPTR, idx,
          self->size);
  }
  if (idx == 0) {
    return list_unshift(self, data);
  }
  if (idx == self->size) {
    return list_push(self, data);
  }
  list_node_t *node = create_list_node(self->allocator, data);
  list_node_t *prev_node;
  list_node_t *next_node;
  if (idx < self->size / 2) {
    prev_node = self->head;
    for (size_t i = 0; i < idx - 1; i++) {
      prev_node = prev_node->next;
    }
    next_node = prev_node->next;
  } else {
    next_node = self->tail;
    for (size_t i = self->size - 1; i > idx; i--) {
      next_node = next_node->prev;
    }
    prev_node = next_node->prev;
  }
  node->prev = prev_node;
  node->next = next_node;
  prev_node->next = node;
  next_node->prev = node;
  self->size++;
  return self->size;
}

size_t list_remove(list_t self, size_t idx) {
  if (idx >= self->size) {
    THROW((size_t)-1,
          "RangeError: index %" PRIuPTR " out of list length %" PRIuPTR, idx,
          self->size);
  }
  list_node_t *node;
  if (idx == 0) {
    list_shift(self);
    return self->size;
  }
  if (idx == self->size - 1) {
    list_pop(self);
    return self->size;
  }
  if (idx < self->size / 2) {
    node = self->head;
    for (size_t i = 0; i < idx; i++) {
      node = node->next;
    }
  } else {
    node = self->tail;
    for (size_t i = self->size - 1; i > idx; i--) {
      node = node->prev;
    }
  }
  list_node_t *prev_node = node->prev;
  list_node_t *next_node = node->next;
  prev_node->next = next_node;
  next_node->prev = prev_node;
  if (self->auto_dispose) {
    allocator_free(self->allocator, node->data);
  }
  allocator_free(self->allocator, node);
  self->size--;
  return self->size;
}

size_t list_clear(list_t self) {
  list_node_t *node = self->head;
  while (node != NULL) {
    list_node_t *next = node->next;
    if (self->auto_dispose) {
      allocator_free(self->allocator, node->data);
    }
    allocator_free(self->allocator, node);
    node = next;
  }
  self->head = NULL;
  self->tail = NULL;
  self->size = 0;
  return self->size;
}

list_iter_t list_iter_first(list_t list) {
  list_iter_t iter = {
      .list = list,
      .current = NULL,
  };
  if (list->head == NULL) {
    return iter;
  }
  iter.current = list->head;
  return iter;
}

void *list_iter_next(list_iter_t *iter) {
  if (iter->current == NULL) {
    return NULL;
  }
  list_node_t *node = (list_node_t *)iter->current;
  void *data = node->data;
  iter->current = node->next;
  return data;
}