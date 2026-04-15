#include "core/hash_map.h"
#include "core/allocator.h"
#include "core/compare.h"
#include "core/hash.h"
#include "core/list.h"
#include <stdint.h>
typedef struct _hash_map_node_t *hash_map_node_t;
struct _hash_map_node_t {
  void *key;
  void *value;
  hash_map_t map;
};
typedef struct _hash_map_chunk_t *hash_map_chunk_t;
struct _hash_map_chunk_t {
  list_t list;
  size_t size;
};
struct _hash_map_t {
  bool autofree_key;
  bool autofree_value;
  allocator_t allocator;
  list_t keys;
  hash_fn_t hash;
  compare_fn_t compare;
  hash_map_chunk_t data;
  size_t num_chunk;
  size_t size;
};

static void hash_map_dispose(hash_map_t self, allocator_t allocator) {
  allocator_free(allocator, self->keys);
  for (size_t idx = 0; idx < self->num_chunk; idx++) {
    hash_map_chunk_t chunk = &self->data[idx];
    allocator_free(allocator, chunk->list);
  }
  allocator_free(allocator, self->data);
}

hash_map_t create_hash_map(allocator_t allocator,
                           hash_map_initialize_t *initialize) {
  hash_map_t self = allocator_alloc(allocator, sizeof(struct _hash_map_t),
                                    (dispose_fn_t)hash_map_dispose);
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
  self->data =
      allocator_alloc(allocator, sizeof(struct _hash_map_chunk_t) * 16, NULL);
  for (size_t idx = 0; idx < self->num_chunk; idx++) {
    self->data[idx].size = 0;
    list_initialize_t initialize = {
        .autofree = true,
    };
    self->data[idx].list = create_list(allocator, &initialize);
  }
  list_initialize_t list_init = {
      .autofree = false,
  };
  self->keys = create_list(allocator, &list_init);
  return self;
}

static void hash_map_node_dispose(hash_map_node_t self, allocator_t allocator) {
  if (self->map->autofree_key) {
    allocator_free(allocator, self->key);
  }
  if (self->map->autofree_value) {
    allocator_free(allocator, self->value);
  }
}
static hash_map_node_t create_hash_map_node(allocator_t allocator,
                                            hash_map_t map, void *key,
                                            void *value) {
  hash_map_node_t self =
      allocator_alloc(allocator, sizeof(struct _hash_map_node_t),
                      (dispose_fn_t)hash_map_node_dispose);
  self->key = key;
  self->value = value;
  self->map = map;
  return self;
}

static hash_map_node_t hash_map_find(hash_map_t self, const void *key,
                                     void *hash_arg, void *cmp_arg) {
  int64_t hash = 0;
  if (self->hash) {
    hash = self->hash(key, hash_arg);
  } else {
    hash = (intptr_t)key;
  }
  int64_t idx = hash % self->num_chunk;
  hash_map_chunk_t chunk = &self->data[idx];
  list_node_t it = list_get_first(chunk->list);
  while (it != list_get_end(chunk->list)) {
    hash_map_node_t n = list_node_get(it);
    if (self->compare(key, n->key, cmp_arg) == 0) {
      return n;
    }
    it = list_node_next(it);
  }
  return NULL;
}
void hash_map_set(hash_map_t self, void *key, void *value, void *hash_arg,
                  void *cmp_arg) {
  hash_map_node_t node = hash_map_find(self, key, hash_arg, cmp_arg);
  if (!node) {
    int64_t hash = 0;
    if (self->hash) {
      hash = self->hash(key, hash_arg);
    } else {
      hash = (intptr_t)key;
    }
    int64_t idx = hash % self->num_chunk;
    hash_map_chunk_t chunk = &self->data[idx];
    node = create_hash_map_node(self->allocator, self, key, value);
    list_append(chunk->list, node);
    chunk->size++;
    list_append(self->keys, node);
    self->size++;
  } else {
    if (self->autofree_value && node->value != value) {
      allocator_free(self->allocator, node->value);
    }
    node->value = value;
  }
}
void *hash_map_get(hash_map_t self, const void *key, void *hash_arg,
                   void *cmp_arg) {
  hash_map_node_t node = hash_map_find(self, key, hash_arg, cmp_arg);
  if (node) {
    return node->value;
  }
  return NULL;
}
void hash_map_delete(hash_map_t self, const void *key, void *hash_arg,
                     void *cmp_arg) {
  int64_t hash = 0;
  if (self->hash) {
    hash = self->hash(key, hash_arg);
  } else {
    hash = (intptr_t)key;
  }
  int64_t idx = hash % self->num_chunk;
  hash_map_chunk_t chunk = &self->data[idx];
  list_node_t it = list_get_first(self->keys);
  while (it != list_get_end(self->keys)) {
    hash_map_node_t n = list_node_get(it);
    if (self->compare(n->key, key, cmp_arg) == 0) {
      list_erase(self->keys, it);
      break;
    }
    it = list_node_next(it);
  }
  it = list_get_first(chunk->list);
  while (it != list_get_end(chunk->list)) {
    hash_map_node_t n = list_node_get(it);
    if (self->compare(n->key, key, cmp_arg) == 0) {
      list_erase(chunk->list, it);
      chunk->size--;
      self->size--;
      break;
    }
    it = list_node_next(it);
  }
}
bool hash_map_has(hash_map_t self, const void *key, void *hash_arg,
                  void *cmp_arg) {
  return hash_map_find(self, key, hash_arg, cmp_arg) != NULL;
}
void *hash_map_move(hash_map_t self, const void *key, void *hash_arg,
                    void *cmp_arg) {
  hash_map_node_t node = hash_map_find(self, key, hash_arg, cmp_arg);
  if (node) {
    void *data = node->value;
    node->value = NULL;
    hash_map_delete(self, key, hash_arg, cmp_arg);
    return data;
  }
  return NULL;
}
void hash_map_clear(hash_map_t self) {
  list_clear(self->keys);
  for (size_t idx = 0; idx < self->num_chunk; idx++) {
    list_clear(self->data[idx].list);
    self->data[idx].size = 0;
  }
  self->size = 0;
}
size_t hash_map_get_size(hash_map_t self) { return self->size; }
list_node_t hash_map_get_begin(hash_map_t self) {
  return list_get_begin(self->keys);
}
list_node_t hash_map_get_end(hash_map_t self) {
  return list_get_end(self->keys);
}
list_node_t hash_map_get_first(hash_map_t self) {
  return list_get_first(self->keys);
}
list_node_t hash_map_get_last(hash_map_t self) {
  return list_get_last(self->keys);
}
list_node_t hash_map_node_get_next(list_node_t self) {
  return list_node_next(self);
}
list_node_t hash_map_node_get_last(list_node_t self) {
  return list_node_last(self);
}
void *hash_map_node_get_key(list_node_t self) {

  hash_map_node_t node = list_node_get(self);
  return node->key;
}
void *hash_map_node_get_value(list_node_t self) {
  hash_map_node_t node = list_node_get(self);
  return node->value;
}
void hash_map_node_set_key(list_node_t self, hash_map_t map, void *key) {
  hash_map_node_t node = list_node_get(self);
  if (node->key != key && map->autofree_key) {
    allocator_free(map->allocator, node->key);
  }
  node->key = key;
}
void hash_map_node_set_value(list_node_t self, hash_map_t map, void *value) {
  hash_map_node_t node = list_node_get(self);
  if (node->value != value && map->autofree_value) {
    allocator_free(map->allocator, node->value);
  }
  node->value = value;
}