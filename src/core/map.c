#include "core/map.h"
#include "core/allocator.h"
#include "core/compare.h"
#include "core/list.h"
#include <stdbool.h>
struct _map_t {
  bool autofree_key;
  bool autofree_value;
  compare_fn_t compare;
  list_t data;
  allocator_t allocator;
};

typedef struct _map_node_t {
  void *key;
  void *value;
} *map_node_t;

static void map_node_dispose(map_node_t self, allocator_t allocator) {}

static map_node_t create_map_node(allocator_t allocator) {
  map_node_t node = allocator_alloc(allocator, sizeof(struct _map_node_t),
                                    (dispose_fn_t)map_node_dispose);
  node->key = NULL;
  node->value = NULL;
  return node;
}
static void map_dispose(map_t self, allocator_t allocator) {
  map_clear(self);
  allocator_free(allocator, self->data);
}

map_t create_map(allocator_t allocator, map_initialize_t *initialize) {
  map_t map = allocator_alloc(allocator, sizeof(struct _map_t),
                              (dispose_fn_t)map_dispose);
  map->autofree_key = false;
  map->autofree_value = false;
  map->compare = NULL;
  if (initialize) {
    map->autofree_key = initialize->autofree_key;
    map->autofree_value = initialize->autofree_value;
    map->compare = initialize->compare;
  }
  list_initialize_t linit = {
      .autofree = true,
      .compare = NULL,
  };
  map->data = create_list(allocator, &linit);
  map->allocator = allocator;
  return map;
}
list_node_t map_find(map_t self, const void *key, void *cmp_arg) {
  list_node_t it = list_get_first(self->data);
  while (it != list_get_end(self->data)) {
    map_node_t node = list_node_get(it);
    if (self->compare) {
      if (self->compare(node->key, key, cmp_arg) == 0) {
        return it;
      }
    } else {
      if (node->key == key) {
        return it;
      }
    }
    it = list_node_next(it);
  }
  return NULL;
}

void map_set(map_t self, void *key, void *value, void *cmp_arg) {
  list_node_t it = map_find(self, key, cmp_arg);
  if (!it) {
    map_node_t node = create_map_node(self->allocator);
    node->key = key;
    node->value = value;
    list_append(self->data, node);
    it = list_get_last(self->data);
  } else {
    map_node_t node = list_node_get(it);
    if (node->key != key && self->autofree_key) {
      allocator_free(self->allocator, node->key);
    }
    node->key = key;
    if (node->value && node->value != value && self->autofree_value) {
      allocator_free(self->allocator, node->value);
    }
    node->value = value;
  }
}
void *map_get(map_t self, const void *key, void *cmp_arg) {
  list_node_t it = map_find(self, key, cmp_arg);
  if (it) {
    map_node_t node = list_node_get(it);
    return node->value;
  }
  return NULL;
}
void map_delete(map_t self, const void *key, void *cmp_arg) {
  list_node_t it = map_find(self, key, cmp_arg);
  if (it) {
    map_node_t node = list_node_get(it);
    if (self->autofree_key) {
      allocator_free(self->allocator, node->key);
    }
    if (self->autofree_value) {
      allocator_free(self->allocator, node->value);
    }
    list_erase(self->data, it);
  }
}
bool map_has(map_t self, const void *key, void *cmp_arg) {
  return map_find(self, key, cmp_arg) != NULL;
}
void map_clear(map_t self) {
  while (list_get_size(self->data)) {
    list_node_t it = list_get_first(self->data);
    map_node_t node = list_node_get(it);
    if (self->autofree_key) {
      allocator_free(self->allocator, node->key);
    }
    if (self->autofree_value) {
      allocator_free(self->allocator, node->value);
    }
    list_erase(self->data, it);
  }
}
size_t map_get_size(map_t self) { return list_get_size(self->data); }
list_node_t map_get_begin(map_t self) { return list_get_begin(self->data); }
list_node_t map_get_end(map_t self) { return list_get_end(self->data); }
list_node_t map_get_first(map_t self) { return list_get_first(self->data); }
list_node_t map_get_last(map_t self) { return list_get_last(self->data); }
list_node_t map_node_get_next(list_node_t self) { return list_node_next(self); }
list_node_t map_node_get_last(list_node_t self) { return list_node_last(self); }
void *map_node_get_key(list_node_t self) {
  return ((map_node_t)list_node_get(self))->key;
}
void *map_node_get_value(list_node_t self) {
  return ((map_node_t)list_node_get(self))->value;
}
void map_node_set_key(list_node_t self, map_t map, void *key) {
  map_node_t node = list_node_get(self);
  if (node->key && map->autofree_key && node->key != key) {
    allocator_free(map->allocator, node->key);
  }
  node->key = key;
}
void map_node_set_value(list_node_t self, map_t map, void *value) {
  map_node_t node = list_node_get(self);
  if (node->value && map->autofree_value && value != node->value) {
    allocator_free(map->allocator, node->value);
  }
  node->value = value;
}
void *map_move(map_t map, const void *key, void *cmp_arg) {
  list_node_t it = map_find(map, key, cmp_arg);
  if (it) {
    map_node_t node = list_node_move(it);
    void *res = node->value;
    node->value = NULL;
    if (map->autofree_key) {
      allocator_free(map->allocator, node->key);
    }
    allocator_free(map->allocator, node);
    list_erase(map->data, it);
    return res;
  }
  return NULL;
}