#include "core/list.h"
#include "core/allocator.h"
#include "core/compare.h"
#include <string.h>

struct _cubec_list_node_t {
  cubec_list_node_t next;
  cubec_list_node_t last;
  void *data;
};
struct _cubec_list_t {
  bool autofree;
  struct _cubec_list_node_t begin;
  struct _cubec_list_node_t end;
  size_t size;
  cubec_compare_fn_t compare;
};

static void cubec_list_dispose(cubec_list_t self, cubec_allocator_t allocator) {
  cubec_list_clear(self, allocator);
}

static void cubec_list_node_dispose(cubec_list_node_t self,
                                    cubec_allocator_t allocator) {
  if (self->last) {
    self->last->next = self->next;
  }
  if (self->next) {
    self->next->last = self->last;
  }
}
static cubec_list_node_t cubec_create_list_node(cubec_allocator_t allocator) {
  cubec_list_node_t node =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_list_node_t),
                            (cubec_dispose_fn_t)cubec_list_node_dispose);
  node->data = NULL;
  node->last = NULL;
  node->next = NULL;
  return node;
}

cubec_list_t cubec_create_list(cubec_allocator_t allocator,
                               cubec_list_initialize_t *initialize) {
  cubec_list_t list =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_list_t),
                            (cubec_dispose_fn_t)cubec_list_dispose);
  list->autofree = false;
  list->compare = NULL;
  if (initialize) {
    list->autofree = initialize->autofree;
    list->compare = initialize->compare;
  }
  list->size = 0;
  memset(&list->begin, 0, sizeof(struct _cubec_list_node_t));
  memset(&list->end, 0, sizeof(struct _cubec_list_node_t));
  list->begin.next = &list->end;
  list->end.last = &list->begin;
  return list;
}

cubec_list_node_t cubec_list_get_begin(cubec_list_t self) {
  return &self->begin;
}

cubec_list_node_t cubec_list_get_end(cubec_list_t self) { return &self->end; }

cubec_list_node_t cubec_list_get_first(cubec_list_t self) {

  return self->begin.next;
}

cubec_list_node_t cubec_list_get_last(cubec_list_t self) {

  return self->end.last;
}

size_t cubec_list_get_size(cubec_list_t self) { return self->size; }

void cubec_list_clear(cubec_list_t self, cubec_allocator_t allocator) {
  while (self->size) {
    if (self->autofree) {
      cubec_allocator_free(allocator, self->begin.next->data);
    }
    cubec_allocator_free(allocator, self->begin.next);
    self->size--;
  }
}

void cubec_list_set_data(cubec_list_t self, cubec_allocator_t allocator,
                         cubec_list_node_t node, void *data) {
  if (node->data == data) {
    return;
  }
  if (self->autofree) {
    cubec_allocator_free(allocator, node->data);
  }
  node->data = data;
}

void cubec_list_append(cubec_list_t self, cubec_allocator_t allocator,
                       void *data) {
  cubec_list_node_t position = self->end.last;
  cubec_list_insert(self, allocator, position, data);
}

void cubec_list_insert(cubec_list_t self, cubec_allocator_t allocator,
                       cubec_list_node_t position, void *data) {
  cubec_list_node_t node = cubec_create_list_node(allocator);
  node->data = data;
  node->last = position;
  node->next = position->next;
  node->last->next = node;
  node->next->last = node;
  self->size++;
}

void cubec_list_erase(cubec_list_t self, cubec_allocator_t allocator,
                      cubec_list_node_t position) {
  if (self->autofree) {
    cubec_allocator_free(allocator, position->data);
  }
  cubec_allocator_free(allocator, position);
  self->size--;
}

cubec_list_node_t cubec_list_find(cubec_list_t self, const void *data,
                                  void *cmp_arg) {
  cubec_list_node_t it = self->begin.next;
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

cubec_list_node_t cubec_list_node_next(cubec_list_node_t self) {
  return self->next;
}

cubec_list_node_t cubec_list_node_last(cubec_list_node_t self) {
  return self->last;
}

void *cubec_list_node_get(cubec_list_node_t self) { return self->data; }
void *cubec_list_node_move(cubec_list_node_t self) {
  void *data = self->data;
  self->data = NULL;
  return data;
}