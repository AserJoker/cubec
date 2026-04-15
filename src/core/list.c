#include "core/list.h"
#include "core/allocator.h"
#include "core/compare.h"
#include <string.h>

struct _list_node_t {
  list_node_t next;
  list_node_t last;
  void *data;
};
struct _list_t {
  bool autofree;
  struct _list_node_t begin;
  struct _list_node_t end;
  size_t size;
  compare_fn_t compare;
  allocator_t allocator;
};

static void list_dispose(list_t self, allocator_t allocator) {
  list_clear(self);
}

static void list_node_dispose(list_node_t self, allocator_t allocator) {
  if (self->last) {
    self->last->next = self->next;
  }
  if (self->next) {
    self->next->last = self->last;
  }
}
static list_node_t create_list_node(allocator_t allocator) {
  list_node_t node = allocator_alloc(allocator, sizeof(struct _list_node_t),
                                     (dispose_fn_t)list_node_dispose);
  node->data = NULL;
  node->last = NULL;
  node->next = NULL;
  return node;
}

list_t create_list(allocator_t allocator, list_initialize_t *initialize) {
  list_t list = allocator_alloc(allocator, sizeof(struct _list_t),
                                (dispose_fn_t)list_dispose);
  list->autofree = false;
  list->compare = NULL;
  list->allocator = allocator;
  if (initialize) {
    list->autofree = initialize->autofree;
    list->compare = initialize->compare;
  }
  list->size = 0;
  memset(&list->begin, 0, sizeof(struct _list_node_t));
  memset(&list->end, 0, sizeof(struct _list_node_t));
  list->begin.next = &list->end;
  list->end.last = &list->begin;
  return list;
}

list_node_t list_get_begin(list_t self) { return &self->begin; }

list_node_t list_get_end(list_t self) { return &self->end; }

list_node_t list_get_first(list_t self) { return self->begin.next; }

list_node_t list_get_last(list_t self) { return self->end.last; }

size_t list_get_size(list_t self) { return self->size; }

void list_clear(list_t self) {
  while (self->size) {
    if (self->autofree) {
      allocator_free(self->allocator, self->begin.next->data);
    }
    allocator_free(self->allocator, self->begin.next);
    self->size--;
  }
}

void list_set_data(list_t self, list_node_t node, void *data) {
  if (node->data == data) {
    return;
  }
  if (self->autofree) {
    allocator_free(self->allocator, node->data);
  }
  node->data = data;
}

void list_append(list_t self, void *data) {
  list_node_t position = self->end.last;
  list_insert(self, position, data);
}

void list_insert(list_t self, list_node_t position, void *data) {
  list_node_t node = create_list_node(self->allocator);
  node->data = data;
  node->last = position;
  node->next = position->next;
  node->last->next = node;
  node->next->last = node;
  self->size++;
}

void list_erase(list_t self, list_node_t position) {
  if (self->autofree) {
    allocator_free(self->allocator, position->data);
  }
  allocator_free(self->allocator, position);
  self->size--;
}

list_node_t list_find(list_t self, const void *data, void *cmp_arg) {
  list_node_t it = self->begin.next;
  while (it != &self->end) {
    if (self->compare && self->compare(data, it->data, cmp_arg) == 0) {
      return it;
    } else if (!self->compare && data == it->data) {
      return it;
    }
    it = it->next;
  }
  return NULL;
}

list_node_t list_node_next(list_node_t self) { return self->next; }

list_node_t list_node_last(list_node_t self) { return self->last; }

void *list_node_get(list_node_t self) { return self->data; }
void *list_node_move(list_node_t self) {
  void *data = self->data;
  self->data = NULL;
  return data;
}