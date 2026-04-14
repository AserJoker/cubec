#include "core/hash_map.h"
#include "core/allocator.h"
#include "core/compare.h"
#include "core/hash.h"
#include "core/list.h"
#include <stdint.h>
typedef struct _cubec_hash_map_node_t *cubec_hash_map_node_t;
struct _cubec_hash_map_node_t {
  void *key;
  void *value;
  cubec_hash_map_t map;
};
typedef struct _cubec_hash_map_chunk_t *cubec_hash_map_chunk_t;
struct _cubec_hash_map_chunk_t {
  cubec_list_t list;
  size_t size;
};
struct _cubec_hash_map_t {
  bool autofree_key;
  bool autofree_value;
  cubec_allocator_t allocator;
  cubec_list_t keys;
  cubec_hash_fn_t hash;
  cubec_compare_fn_t compare;
  cubec_hash_map_chunk_t data;
  size_t num_chunk;
  size_t size;
};

static void cubec_hash_map_dispose(cubec_hash_map_t self,
                                   cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->keys);
  for (size_t idx = 0; idx < self->num_chunk; idx++) {
    cubec_hash_map_chunk_t chunk = &self->data[idx];
    cubec_allocator_free(allocator, chunk->list);
  }
  cubec_allocator_free(allocator, self->data);
}

cubec_hash_map_t
cubec_create_hash_map(cubec_allocator_t allocator,
                      cubec_hash_map_initialize_t *initialize) {
  cubec_hash_map_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_hash_map_t),
                            (cubec_dispose_fn_t)cubec_hash_map_dispose);
  self->allocator = allocator;
  self->size = 0;
  self->autofree_key = false;
  self->autofree_value = false;
  self->compare = NULL;
  self->hash = NULL;
  if (initialize) {
    self->hash = initialize->hash;
    self->autofree_key = initialize->autofree_key;
    self->autofree_value = initialize->autofree_value;
    self->compare = initialize->compare;
  }
  self->num_chunk = 16;
  self->data = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_hash_map_chunk_t) * 16, NULL);
  for (size_t idx = 0; idx < self->num_chunk; idx++) {
    self->data[idx].size = 0;
    cubec_list_initialize_t initialize = {
        .autofree = true,
    };
    self->data[idx].list = cubec_create_list(allocator, &initialize);
  }
  cubec_list_initialize_t list_init = {
      .autofree = false,
  };
  self->keys = cubec_create_list(allocator, &list_init);
  return self;
}

static void cubec_hash_map_node_dispose(cubec_hash_map_node_t self,
                                        cubec_allocator_t allocator) {
  if (self->map->autofree_key) {
    cubec_allocator_free(allocator, self->key);
  }
  if (self->map->autofree_value) {
    cubec_allocator_free(allocator, self->value);
  }
}
static cubec_hash_map_node_t
cubec_create_hash_map_node(cubec_allocator_t allocator, cubec_hash_map_t map,
                           void *key, void *value) {
  cubec_hash_map_node_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_hash_map_node_t),
                            (cubec_dispose_fn_t)cubec_hash_map_node_dispose);
  self->key = key;
  self->value = value;
  self->map = map;
  return self;
}

static cubec_hash_map_node_t cubec_hash_map_find(cubec_hash_map_t self,
                                                 const void *key,
                                                 void *hash_arg,
                                                 void *cmp_arg) {
  int64_t hash = 0;
  if (self->hash) {
    hash = self->hash(key, hash_arg);
  } else {
    hash = (intptr_t)key;
  }
  int64_t idx = hash % self->num_chunk;
  cubec_hash_map_chunk_t chunk = &self->data[idx];
  cubec_list_node_t it = cubec_list_get_first(chunk->list);
  while (it != cubec_list_get_end(chunk->list)) {
    cubec_hash_map_node_t n = cubec_list_node_get(it);
    if (self->compare(key, n->key, cmp_arg) == 0) {
      return n;
    }
    it = cubec_list_node_next(it);
  }
  return NULL;
}
void cubec_hash_map_set(cubec_hash_map_t self, void *key, void *value,
                        void *hash_arg, void *cmp_arg) {
  cubec_hash_map_node_t node =
      cubec_hash_map_find(self, key, hash_arg, cmp_arg);
  if (!node) {
    int64_t hash = 0;
    if (self->hash) {
      hash = self->hash(key, hash_arg);
    } else {
      hash = (intptr_t)key;
    }
    int64_t idx = hash % self->num_chunk;
    cubec_hash_map_chunk_t chunk = &self->data[idx];
    node = cubec_create_hash_map_node(self->allocator, self, key, value);
    cubec_list_append(chunk->list, node);
    chunk->size++;
    cubec_list_append(self->keys, node);
    self->size++;
  } else {
    if (self->autofree_value && node->value != value) {
      cubec_allocator_free(self->allocator, node->value);
    }
    node->value = value;
  }
}
void *cubec_hash_map_get(cubec_hash_map_t self, const void *key, void *hash_arg,
                         void *cmp_arg) {
  cubec_hash_map_node_t node =
      cubec_hash_map_find(self, key, hash_arg, cmp_arg);
  if (node) {
    return node->value;
  }
  return NULL;
}
void cubec_hash_map_delete(cubec_hash_map_t self, const void *key,
                           void *hash_arg, void *cmp_arg) {
  int64_t hash = 0;
  if (self->hash) {
    hash = self->hash(key, hash_arg);
  } else {
    hash = (intptr_t)key;
  }
  int64_t idx = hash % self->num_chunk;
  cubec_hash_map_chunk_t chunk = &self->data[idx];
  cubec_list_node_t it = cubec_list_get_first(self->keys);
  while (it != cubec_list_get_end(self->keys)) {
    cubec_hash_map_node_t n = cubec_list_node_get(it);
    if (self->compare(n->key, key, cmp_arg) == 0) {
      cubec_list_erase(self->keys, it);
      break;
    }
    it = cubec_list_node_next(it);
  }
  it = cubec_list_get_first(chunk->list);
  while (it != cubec_list_get_end(chunk->list)) {
    cubec_hash_map_node_t n = cubec_list_node_get(it);
    if (self->compare(n->key, key, cmp_arg) == 0) {
      cubec_list_erase(chunk->list, it);
      chunk->size--;
      self->size--;
      break;
    }
    it = cubec_list_node_next(it);
  }
}
bool cubec_hash_map_has(cubec_hash_map_t self, const void *key, void *hash_arg,
                        void *cmp_arg) {
  return cubec_hash_map_find(self, key, hash_arg, cmp_arg) != NULL;
}
void *cubec_hash_map_move(cubec_hash_map_t self, const void *key,
                          void *hash_arg, void *cmp_arg) {
  cubec_hash_map_node_t node =
      cubec_hash_map_find(self, key, hash_arg, cmp_arg);
  if (node) {
    void *data = node->value;
    node->value = NULL;
    cubec_hash_map_delete(self, key, hash_arg, cmp_arg);
    return data;
  }
  return NULL;
}
void cubec_hash_map_clear(cubec_hash_map_t self) {
  cubec_list_clear(self->keys);
  for (size_t idx = 0; idx < self->num_chunk; idx++) {
    cubec_list_clear(self->data[idx].list);
    self->data[idx].size = 0;
  }
  self->size = 0;
}
size_t cubec_hash_map_get_size(cubec_hash_map_t self) { return self->size; }
cubec_list_node_t cubec_hash_map_get_begin(cubec_hash_map_t self) {
  return cubec_list_get_begin(self->keys);
}
cubec_list_node_t cubec_hash_map_get_end(cubec_hash_map_t self) {
  return cubec_list_get_end(self->keys);
}
cubec_list_node_t cubec_hash_map_get_first(cubec_hash_map_t self) {
  return cubec_list_get_first(self->keys);
}
cubec_list_node_t cubec_hash_map_get_last(cubec_hash_map_t self) {
  return cubec_list_get_last(self->keys);
}
cubec_list_node_t cubec_hash_map_node_get_next(cubec_list_node_t self) {
  return cubec_list_node_next(self);
}
cubec_list_node_t cubec_hash_map_node_get_last(cubec_list_node_t self) {
  return cubec_list_node_last(self);
}
void *cubec_hash_map_node_get_key(cubec_list_node_t self) {

  cubec_hash_map_node_t node = cubec_list_node_get(self);
  return node->key;
}
void *cubec_hash_map_node_get_value(cubec_list_node_t self) {
  cubec_hash_map_node_t node = cubec_list_node_get(self);
  return node->value;
}
void cubec_hash_map_node_set_key(cubec_list_node_t self, cubec_hash_map_t map,
                                 void *key) {
  cubec_hash_map_node_t node = cubec_list_node_get(self);
  if (node->key != key && map->autofree_key) {
    cubec_allocator_free(map->allocator, node->key);
  }
  node->key = key;
}
void cubec_hash_map_node_set_value(cubec_list_node_t self, cubec_hash_map_t map,
                                   void *value) {
  cubec_hash_map_node_t node = cubec_list_node_get(self);
  if (node->value != value && map->autofree_value) {
    cubec_allocator_free(map->allocator, node->value);
  }
  node->value = value;
}