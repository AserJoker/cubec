#include "core/map.h"
#include "core/allocator.h"
#include "core/compare.h"
#include "core/list.h"
#include <stdbool.h>
struct _cubec_map_t {
  bool autofree_key;
  bool autofree_value;
  cubec_compare_fn_t compare;
  cubec_list_t data;
  cubec_allocator_t allocator;
};

typedef struct _cubec_map_node_t {
  void *key;
  void *value;
} *cubec_map_node_t;

static void cubec_map_node_dispose(cubec_map_node_t self,
                                   cubec_allocator_t allocator) {}

static cubec_map_node_t cubec_create_map_node(cubec_allocator_t allocator) {
  cubec_map_node_t node =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_map_node_t),
                            (cubec_dispose_fn_t)cubec_map_node_dispose);
  node->key = NULL;
  node->value = NULL;
  return node;
}
static void cubec_map_dispose(cubec_map_t self, cubec_allocator_t allocator) {
  cubec_map_clear(self);
  cubec_allocator_free(allocator, self->data);
}

cubec_map_t cubec_create_map(cubec_allocator_t allocator,
                             cubec_map_initialize_t *initialize) {
  cubec_map_t map =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_map_t),
                            (cubec_dispose_fn_t)cubec_map_dispose);
  map->autofree_key = false;
  map->autofree_value = false;
  map->compare = NULL;
  if (initialize) {
    map->autofree_key = initialize->autofree_key;
    map->autofree_value = initialize->autofree_value;
    map->compare = initialize->compare;
  }
  cubec_list_initialize_t linit = {
      .autofree = true,
      .compare = NULL,
  };
  map->data = cubec_create_list(allocator, &linit);
  map->allocator = allocator;
  return map;
}
cubec_list_node_t cubec_map_find(cubec_map_t self, const void *key,
                                 void *cmp_arg) {
  cubec_list_node_t it = cubec_list_get_first(self->data);
  while (it != cubec_list_get_end(self->data)) {
    cubec_map_node_t node = cubec_list_node_get(it);
    if (self->compare) {
      if (self->compare(node->key, key, cmp_arg) == 0) {
        return it;
      }
    } else {
      if (node->key == key) {
        return it;
      }
    }
    it = cubec_list_node_next(it);
  }
  return NULL;
}

void cubec_map_set(cubec_map_t self, void *key, void *value, void *cmp_arg) {
  cubec_list_node_t it = cubec_map_find(self, key, cmp_arg);
  if (!it) {
    cubec_map_node_t node = cubec_create_map_node(self->allocator);
    node->key = key;
    cubec_list_append(self->data, node);
    it = cubec_list_get_last(self->data);
  }
  cubec_map_node_t node = cubec_list_node_get(it);
  if (node->value && node->value != value && self->autofree_value) {
    cubec_allocator_free(self->allocator, node->value);
  }
  node->value = value;
}
void *cubec_map_get(cubec_map_t self, const void *key, void *cmp_arg) {
  cubec_list_node_t it = cubec_map_find(self, key, cmp_arg);
  if (it) {
    cubec_map_node_t node = cubec_list_node_get(it);
    return node->value;
  }
  return NULL;
}
void cubec_map_delete(cubec_map_t self, const void *key, void *cmp_arg) {
  cubec_list_node_t it = cubec_map_find(self, key, cmp_arg);
  if (it) {
    cubec_map_node_t node = cubec_list_node_get(it);
    if (self->autofree_key) {
      cubec_allocator_free(self->allocator, node->key);
    }
    if (self->autofree_value) {
      cubec_allocator_free(self->allocator, node->value);
    }
    cubec_list_erase(self->data, it);
  }
}
bool cubec_map_has(cubec_map_t self, const void *key, void *cmp_arg) {
  return cubec_map_find(self, key, cmp_arg) != NULL;
}
void cubec_map_clear(cubec_map_t self) {
  while (cubec_list_get_size(self->data)) {
    cubec_list_node_t it = cubec_list_get_first(self->data);
    cubec_map_node_t node = cubec_list_node_get(it);
    if (self->autofree_key) {
      cubec_allocator_free(self->allocator, node->key);
    }
    if (self->autofree_value) {
      cubec_allocator_free(self->allocator, node->value);
    }
    cubec_list_erase(self->data, it);
  }
}
size_t cubec_map_get_size(cubec_map_t self) {
  return cubec_list_get_size(self->data);
}
cubec_list_node_t cubec_map_get_begin(cubec_map_t self) {
  return cubec_list_get_begin(self->data);
}
cubec_list_node_t cubec_map_get_end(cubec_map_t self) {
  return cubec_list_get_end(self->data);
}
cubec_list_node_t cubec_map_get_first(cubec_map_t self) {
  return cubec_list_get_first(self->data);
}
cubec_list_node_t cubec_map_get_last(cubec_map_t self) {
  return cubec_list_get_last(self->data);
}
cubec_list_node_t cubec_map_node_get_next(cubec_list_node_t self) {
  return cubec_list_node_next(self);
}
cubec_list_node_t cubec_map_node_get_last(cubec_list_node_t self) {
  return cubec_list_node_last(self);
}
void *cubec_map_node_get_key(cubec_list_node_t self) {
  return ((cubec_map_node_t)cubec_list_node_get(self))->key;
}
void *cubec_map_node_get_value(cubec_list_node_t self) {
  return ((cubec_map_node_t)cubec_list_node_get(self))->value;
}
void cubec_map_node_set_key(cubec_list_node_t self, cubec_map_t map,
                            void *key) {
  cubec_map_node_t node = cubec_list_node_get(self);
  if (node->key && map->autofree_key && node->key != key) {
    cubec_allocator_free(map->allocator, node->key);
  }
  node->key = key;
}
void cubec_map_node_set_value(cubec_list_node_t self, cubec_map_t map,
                              void *value) {
  cubec_map_node_t node = cubec_list_node_get(self);
  if (node->value && map->autofree_value && value != node->value) {
    cubec_allocator_free(map->allocator, node->value);
  }
  node->value = value;
}