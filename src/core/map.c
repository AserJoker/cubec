#include "core/map.h"
#include "core/allocator.h"
#include "core/compare.h"
#include <stdbool.h>
struct _cubec_map_t {
  bool autofree_key;
  bool autofree_value;
  size_t size;
  cubec_map_node_t begin;
  cubec_map_node_t end;
  cubec_compare_fn_t compare;
};

struct _cubec_map_node_t {
  void *key;
  void *value;
  cubec_map_node_t next;
  cubec_map_node_t last;
};

static void cubec_map_node_dispose(cubec_map_node_t self,
                                   cubec_allocator_t allocator) {
  if (self->last) {
    self->last->next = self->next;
  }
  if (self->next) {
    self->next->last = self->last;
  }
}

static cubec_map_node_t cubec_create_map_node(cubec_allocator_t allocator) {
  cubec_map_node_t node =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_map_node_t),
                            (cubec_dispose_fn_t)cubec_map_node_dispose);
  node->key = NULL;
  node->value = NULL;
  node->next = NULL;
  node->last = NULL;
  return node;
}
static void cubec_map_dispose(cubec_map_t self, cubec_allocator_t allocator) {
  cubec_map_clear(self, allocator);
  cubec_allocator_free(allocator, self->begin);
  cubec_allocator_free(allocator, self->end);
}

cubec_map_t cubec_create_map(cubec_allocator_t allocator,
                             cubec_map_initialize_t *initialize) {
  cubec_map_t map =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_map_t),
                            (cubec_dispose_fn_t)cubec_map_dispose);
  map->autofree_key = false;
  map->autofree_value = false;
  map->compare = NULL;
  map->size = 0;
  if (initialize) {
    map->autofree_key = initialize->autofree_key;
    map->autofree_value = initialize->autofree_value;
    map->compare = initialize->compare;
  }
  map->begin = cubec_create_map_node(allocator);
  map->end = cubec_create_map_node(allocator);
  map->begin->next = map->end;
  map->end->last = map->begin;
  return map;
}
cubec_map_node_t cubec_map_find(cubec_map_t self, void *key, void *cmp_arg) {
  cubec_map_node_t it = self->begin->next;
  while (it != self->end) {
    if (self->compare) {
      if (self->compare(it->key, key, cmp_arg)) {
        return it;
      }
    } else {
      if (it->key == key) {
        return it;
      }
    }
    it = it->next;
  }
  return NULL;
}

void cubec_map_set(cubec_map_t self, cubec_allocator_t allocator, void *key,
                   void *value, void *cmp_arg) {
  cubec_map_node_t node = cubec_map_find(self, key, cmp_arg);
  if (!node) {
    node = cubec_create_map_node(allocator);
    node->key = key;
    node->last = self->end->last;
    node->next = self->end;
    node->next->last = node;
    node->last->next = node;
    self->size++;
  }
  if (node->value && node->value != value && self->autofree_value) {
    cubec_allocator_free(allocator, node->value);
  }
  node->value = value;
}
void *cubec_map_get(cubec_map_t self, void *key, void *cmp_arg) {
  cubec_map_node_t node = cubec_map_find(self, key, cmp_arg);
  if (node) {
    return node->value;
  }
  return NULL;
}
void cubec_map_delete(cubec_map_t self, cubec_allocator_t allocator, void *key,
                      void *cmp_arg) {
  cubec_map_node_t node = cubec_map_find(self, key, cmp_arg);
  if (node) {
    if (self->autofree_key) {
      cubec_allocator_free(allocator, node->key);
    }
    if (self->autofree_value) {
      cubec_allocator_free(allocator, node->value);
    }
    cubec_allocator_free(allocator, node);
    self->size--;
  }
}
bool cubec_map_has(cubec_map_t self, void *key, void *cmp_arg) {
  return cubec_map_find(self, key, cmp_arg) != NULL;
}
size_t cubec_map_get_length(cubec_map_t self) { return self->size; }
cubec_map_node_t cubec_map_get_begin(cubec_map_t self) { return self->begin; }
cubec_map_node_t cubec_map_get_end(cubec_map_t self) { return self->end; }
cubec_map_node_t cubec_map_get_first(cubec_map_t self) {
  return self->begin->next;
}
cubec_map_node_t cubec_map_get_last(cubec_map_t self) {
  return self->end->last;
}
void *cubec_map_node_get_key(cubec_map_node_t self) { return self->key; }
void *cubec_map_node_get_value(cubec_map_node_t self) { return self->value; }
void cubec_map_node_set_key(cubec_map_node_t self, cubec_allocator_t allocator,
                            cubec_map_t map, void *key) {
  if (self->key && map->autofree_key && key != self->key) {
    cubec_allocator_free(allocator, self->key);
  }
  self->key = key;
}
void cubec_map_node_set_value(cubec_map_node_t self,
                              cubec_allocator_t allocator, cubec_map_t map,
                              void *value) {
  if (self->value && map->autofree_value && value != self->value) {
    cubec_allocator_free(allocator, self->value);
  }
  self->value = value;
}