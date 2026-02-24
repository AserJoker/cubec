#include "core/hash_map.h"
#include "core/allocator.h"
#include "core/compare.h"
#include "core/hash.h"
#include "core/list.h"
#include <stdbool.h>
#include <stdint.h>
struct _cubec_hash_map_node_t {
  void *key;
  void *value;
  int64_t hash;
};
struct _cubec_hash_map_t {
  bool autofree_key;
  bool autofree_value;
  cubec_hash_fn_t hash;
  cubec_list_t *buckets;
  size_t capacity;
  size_t length;
  struct _cubec_hash_map_node_t begin;
  struct _cubec_hash_map_node_t end;
};

static void cubec_hash_map_dispose(cubec_hash_map_t self,
                                   cubec_allocator_t allocator) {
  cubec_hash_map_clear(self, allocator);
  for (size_t idx = 0; idx < self->length; idx++) {
    cubec_allocator_free(allocator, self->buckets[idx]);
  }
  cubec_allocator_free(allocator, self->buckets);
}

cubec_hash_map_t
cubec_create_hash_map(cubec_allocator_t allocator,
                      cubec_hash_map_initialize_t *initialize) {
  cubec_hash_map_t hash_map =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_hash_map_t),
                            (cubec_dispose_fn_t)cubec_hash_map_dispose);
  if (initialize) {
    hash_map->autofree_key = initialize->autofree_key;
    hash_map->autofree_value = initialize->autofree_value;
    hash_map->hash = initialize->hash;
  } else {
    hash_map->autofree_key = false;
    hash_map->autofree_value = false;
    hash_map->hash = NULL;
  }
  hash_map->capacity = 8;
  hash_map->buckets = cubec_allocator_alloc(
      allocator, sizeof(cubec_list_t) * hash_map->capacity, NULL);
  for (size_t idx = 0; idx < hash_map->capacity; idx++) {
    hash_map->buckets[idx] = NULL;
  }
  hash_map->length = 0;
  return hash_map;
}
static int32_t cubec_hash_map_compare(const cubec_hash_map_node_t node,
                                      const void *key) {
  return node->key - key;
}
static void cubec_hash_map_node_dispose(cubec_hash_map_node_t self,
                                        cubec_allocator_t allocator) {}

static cubec_hash_map_node_t
cubec_create_hash_map_node(cubec_allocator_t allocator) {
  cubec_hash_map_node_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_hash_map_node_t),
                            (cubec_dispose_fn_t)cubec_hash_map_dispose);
  self->hash = 0;
  self->key = NULL;
  self->value = NULL;
  return self;
}
bool cubec_hash_map_set(cubec_hash_map_t self, cubec_allocator_t allocator,
                        const void *key, void *value, void *hash_arg) {
  int64_t hash = self->hash ? self->hash(key, hash_arg) : (intptr_t)key;
  int64_t idx = hash % self->capacity;
  cubec_list_t entry = self->buckets[idx];
  if (!entry) {
    return false;
  }
  cubec_list_node_t it = cubec_list_find(entry, &hash, NULL);
  if (!it) {
    return false;
  }
  cubec_hash_map_node_t node = cubec_list_node_get(it);
  if (node->value && node->value == value && self->autofree_value) {
    cubec_allocator_free(allocator, node->value);
  }
  node->value = value;
  return true;
}
void *cubec_hash_map_get(cubec_hash_map_t self, cubec_allocator_t allocator,
                         const void *key, void *hash_arg) {
  int64_t hash = self->hash ? self->hash(key, hash_arg) : (intptr_t)key;
  int64_t idx = hash % self->capacity;
  cubec_list_t entry = self->buckets[idx];
  if (!entry) {
    return NULL;
  }
  cubec_list_node_t it = cubec_list_find(entry, &hash, NULL);
  if (!it) {
    return NULL;
  }
  cubec_hash_map_node_t node = cubec_list_node_get(it);
  return node->value;
}
bool cubec_hash_map_has(cubec_hash_map_t self, cubec_allocator_t allocator,
                        const void *key, void *hash_arg) {
  return cubec_hash_map_get(self, allocator, key, hash_arg) != NULL;
}
bool cubec_hash_map_delete(cubec_hash_map_t self, cubec_allocator_t allocator,
                           const void *key, void *hash_arg) {
  int64_t hash = self->hash ? self->hash(key, hash_arg) : (intptr_t)key;
  int64_t idx = hash % self->capacity;
  cubec_list_t entry = self->buckets[idx];
  if (!entry) {
    return false;
  }
  cubec_list_node_t it = cubec_list_find(entry, key, NULL);
  if (!it) {
    return false;
  }
  cubec_hash_map_node_t node = cubec_list_node_get(it);
  if (node->key && self->autofree_key) {
    cubec_allocator_free(allocator, node->key);
  }
  if (node->value && self->autofree_value) {
    cubec_allocator_free(allocator, node->value);
  }
  cubec_list_erase(entry, allocator, it);
  return true;
}
bool cubec_hash_map_put(cubec_hash_map_t self, cubec_allocator_t allocator,
                        void *key, void *value, void *hash_arg) {
  int64_t hash = self->hash ? self->hash(key, hash_arg) : (intptr_t)key;
  int64_t idx = hash % self->capacity;
  cubec_list_t entry = self->buckets[idx];
  if (!entry) {
    cubec_list_initialize_t initiailze = {
        .autofree = true,
        .compare = (cubec_compare_fn_t)cubec_hash_map_compare,
    };
    self->buckets[idx] = cubec_create_list(allocator, &initiailze);
    self->length++;
    entry = self->buckets[idx];
  }
  cubec_list_node_t it = cubec_list_find(entry, key, NULL);
  if (it) {
    return false;
  }
  cubec_hash_map_node_t node = cubec_create_hash_map_node(allocator);
  node->key = key;
  node->value = value;
  node->hash = hash;
  cubec_list_append(entry, allocator, node);
  return true;
}
void cubec_hash_map_clear(cubec_hash_map_t self, cubec_allocator_t allocator) {
  for (size_t idx = 0; idx < self->capacity; idx++) {
    cubec_list_t entry = self->buckets[idx];
    if (!entry) {
      continue;
    }
    cubec_list_node_t it = cubec_list_get_first(entry);
    while (it != cubec_list_get_end(entry)) {
      cubec_hash_map_node_t node = cubec_list_node_get(it);
      if (self->autofree_key) {
        cubec_allocator_free(allocator, node->key);
      }
      if (self->autofree_value) {
        cubec_allocator_free(allocator, node->value);
      }
      it = cubec_list_node_next(it);
    }
    cubec_allocator_free(allocator, self->buckets[idx]);
  }
}
cubec_hash_map_node_t cubec_hash_map_get_begin(cubec_hash_map_t self) {
  return &self->begin;
}
cubec_hash_map_node_t cubec_hash_map_get_end(cubec_hash_map_t self) {
  return &self->end;
}
cubec_hash_map_node_t cubec_hash_map_get_first(cubec_hash_map_t self) {
  for (size_t idx = 0; idx < self->capacity; idx++) {
    if (!self->buckets[idx]) {
      continue;
    }
    cubec_list_t entry = self->buckets[idx];
    if (cubec_list_get_size(entry)) {
      cubec_list_node_t it = cubec_list_get_first(entry);
      return cubec_list_node_get(it);
    }
  }
  return NULL;
}
cubec_hash_map_node_t cubec_hash_map_get_last(cubec_hash_map_t self) {
  for (size_t idx = self->capacity - 1; idx >= 0; idx--) {
    if (!self->buckets[idx]) {
      continue;
    }
    cubec_list_t entry = self->buckets[idx];
    if (cubec_list_get_size(entry)) {
      cubec_list_node_t it = cubec_list_get_last(entry);
      return cubec_list_node_get(it);
    }
  }
  return NULL;
}
cubec_hash_map_node_t cubec_hash_map_node_next(cubec_hash_map_node_t self,
                                               cubec_hash_map_t hmap) {
  if (self == &hmap->begin) {
    return cubec_hash_map_get_first(hmap);
  }
  if (self == &hmap->end) {
    return NULL;
  }
  size_t idx = self->hash % hmap->capacity;
  cubec_list_t entry = hmap->buckets[idx];
  cubec_list_node_t it = cubec_list_find(entry, self->key, NULL);
  it = cubec_list_node_next(it);
  if (it != cubec_list_get_end(entry)) {
    return cubec_list_node_get(it);
  } else {
    idx++;
    entry = NULL;
    while (idx < hmap->capacity) {
      if (hmap->buckets[idx] && cubec_list_get_size(hmap->buckets[idx])) {
        entry = hmap->buckets[idx];
        break;
      }
      idx++;
    }
    if (entry) {
      it = cubec_list_get_first(entry);
      return cubec_list_node_get(it);
    }
  }
  return &hmap->end;
}
cubec_hash_map_node_t cubec_hash_map_node_last(cubec_hash_map_node_t self,
                                               cubec_hash_map_t hmap) {
  if (self == &hmap->end) {
    return cubec_hash_map_get_last(hmap);
  }
  if (self == &hmap->begin) {
    return NULL;
  }
  size_t idx = self->hash % hmap->capacity;
  cubec_list_t entry = hmap->buckets[idx];
  cubec_list_node_t it = cubec_list_find(entry, self->key, NULL);
  it = cubec_list_node_last(it);
  if (it != cubec_list_get_begin(entry)) {
    return cubec_list_node_get(it);
  } else {
    idx--;
    entry = NULL;
    while (idx >= 0) {
      if (hmap->buckets[idx] && cubec_list_get_size(hmap->buckets[idx])) {
        entry = hmap->buckets[idx];
        break;
      }
      idx--;
    }
    if (entry) {
      it = cubec_list_get_last(entry);
      return cubec_list_node_get(it);
    }
  }
  return &hmap->begin;
}
void *cubec_hash_map_get_key(cubec_hash_map_node_t self) { return self->key; }
void *cubec_hash_map_get_value(cubec_hash_map_node_t self) {
  return self->value;
}