#include "core/map.h"
#include "core/allocator.h"
#include "core/vec.h"
#include "core/list.h"
#include "core/rbtree.h"

#define DEFAULT_BUCKET_COUNT 16

typedef struct _map_entry_index_t {
  uint64_t key_id;
  size_t index;
} map_entry_index_t;

struct _map_t {
  allocator_t allocator;
  bool key_auto_dispose;
  bool value_auto_dispose;
  vec_t keys;
  vec_t values;
  map_index_type_t index_type;
  union {
    list_t buckets[DEFAULT_BUCKET_COUNT];
    rbtree_t rbtree;
  } index;
};

static void _map_init(map_t self, allocator_t allocator, map_init_t *init) {
  if (init) {
    self->key_auto_dispose = init->key_auto_dispose;
    self->value_auto_dispose = init->value_auto_dispose;
  } else {
    self->key_auto_dispose = false;
    self->value_auto_dispose = false;
  }
  self->allocator = allocator;
  vec_init_t keys_init = {.auto_dispose = self->key_auto_dispose};
  vec_init_t values_init = {.auto_dispose = self->value_auto_dispose};
  self->keys = allocator_create(allocator, &g_vec_type, &keys_init);
  self->values = allocator_create(allocator, &g_vec_type, &values_init);
  self->index_type = MAP_INDEX_HASH;
  for (size_t i = 0; i < DEFAULT_BUCKET_COUNT; i++) {
    list_init_t list_init = {.auto_dispose = true};
    self->index.buckets[i] = allocator_create(allocator, &g_list_type, &list_init);
  }
}

static void _map_dispose(map_t self, allocator_t allocator) {
  map_clear(self);
  if (self->index_type == MAP_INDEX_HASH) {
    for(size_t idx=0;idx<DEFAULT_BUCKET_COUNT;idx++){
      allocator_free(self->allocator, &self->index.buckets[idx]);
    }
  } else {
    allocator_free(self->allocator, &self->index.rbtree);
  }
  allocator_free(allocator, &self->keys);
  allocator_free(allocator, &self->values);
}

static size_t hash_bucket_index(map_t self, uint64_t key_id) {
  (void)self;
  return key_id % DEFAULT_BUCKET_COUNT;
}

static map_entry_index_t *find_entry_in_list(list_t list, uint64_t key_id) {
  list_iter_t iter = list_iter_first(list);
  map_entry_index_t *entry;
  while ((entry = list_iter_next(&iter)) != NULL) {
    if (entry->key_id == key_id) {
      return entry;
    }
  }
  return NULL;
}

static map_entry_index_t *find_entry_by_key(map_t self, void *key) {
  uint64_t key_id = value_get_id(key);
  if (self->index_type == MAP_INDEX_HASH) {
    size_t bucket_idx = hash_bucket_index(self, key_id);
    return find_entry_in_list(self->index.buckets[bucket_idx], key_id);
  } else {
    return (map_entry_index_t *)rbtree_find(self->index.rbtree, key_id);
  }
}

static void _map_clone(map_t self, allocator_t allocator, map_t another) {
  self->key_auto_dispose = another->key_auto_dispose;
  self->value_auto_dispose = another->value_auto_dispose;
  self->allocator = allocator;
  vec_init_t keys_init = {.auto_dispose = another->key_auto_dispose};
  vec_init_t values_init = {.auto_dispose = another->value_auto_dispose};
  self->keys = allocator_create(allocator, &g_vec_type, &keys_init);
  self->values = allocator_create(allocator, &g_vec_type, &values_init);
  self->index_type = MAP_INDEX_HASH;
  for (size_t i = 0; i < DEFAULT_BUCKET_COUNT; i++) {
    list_init_t list_init = {.auto_dispose = true};
    self->index.buckets[i] = allocator_create(allocator, &g_list_type, &list_init);
  }

  size_t size = vec_get_size(another->keys);
  for (size_t i = 0; i < size; i++) {
    void *cloned_key = value_clone(allocator, vec_get(another->keys, i));
    void *cloned_value = value_clone(allocator, vec_get(another->values, i));
    map_insert(self, cloned_key, cloned_value);
  }
}

static void _map_move(map_t self, allocator_t allocator, map_t another) {
  self->key_auto_dispose = another->key_auto_dispose;
  self->value_auto_dispose = another->value_auto_dispose;
  self->allocator = allocator;
  self->keys = another->keys;
  self->values = another->values;
  self->index_type = another->index_type;
  self->index = another->index;

  another->keys = allocator_create(allocator, &g_vec_type, &(vec_init_t){false});
  another->values = allocator_create(allocator, &g_vec_type, &(vec_init_t){false});
  another->index_type = MAP_INDEX_HASH;
  for (size_t idx = 0; idx < DEFAULT_BUCKET_COUNT; idx++) {
    list_init_t list_init = {.auto_dispose = true};
    another->index.buckets[idx] = allocator_create(allocator, &g_list_type, &list_init);
  }
}

type_t g_map_type = {
    .size = sizeof(struct _map_t),
    .name = "cubec.core.map",
    .init = (type_init_fn_t)_map_init,
    .dispose = (type_dispose_fn_t)_map_dispose,
    .clone = (type_clone_fn_t)_map_clone,
    .move = (type_move_fn_t)_map_move,
};

static map_entry_index_t *create_entry_index(allocator_t allocator,
                                              uint64_t key_id, size_t index) {
  map_entry_index_t *entry = (map_entry_index_t *)allocator_alloc(
      allocator, sizeof(map_entry_index_t));
  entry->key_id = key_id;
  entry->index = index;
  return entry;
}

static void convert_to_rbtree(map_t self) {
  rbtree_init_t init = {
      .auto_dispose = true,
  };

  list_t buckets[DEFAULT_BUCKET_COUNT];
  for (size_t i = 0; i < DEFAULT_BUCKET_COUNT; i++) {
    buckets[i] = self->index.buckets[i];
  }

  self->index.rbtree = allocator_create(self->allocator, &g_rbtree_type, &init);

  for (size_t i = 0; i < DEFAULT_BUCKET_COUNT; i++) {
    list_t bucket = buckets[i];
    list_iter_t iter = list_iter_first(bucket);
    map_entry_index_t *entry;
    while ((entry = list_iter_next(&iter)) != NULL) {
      map_entry_index_t *moved_entry = (map_entry_index_t *)value_move(self->allocator, entry);
      rbtree_insert(self->index.rbtree, moved_entry->key_id, moved_entry);
    }
  }

  for (size_t i = 0; i < DEFAULT_BUCKET_COUNT; i++) {
    allocator_free(self->allocator, &buckets[i]);
  }

  self->index_type = MAP_INDEX_RBTREE;
}

size_t map_get_size(map_t self) { return vec_get_size(self->keys); }

void *map_find(map_t self, void *key) {
  map_entry_index_t *entry = find_entry_by_key(self, key);
  if (entry == NULL) {
    return NULL;
  }
  return vec_get(self->values, entry->index);
}

size_t map_insert(map_t self, void *key, void *value) {
  map_entry_index_t *entry = find_entry_by_key(self, key);
  if (entry != NULL) {
    vec_set(self->values, entry->index, value);
    return vec_get_size(self->keys);
  }

  uint64_t key_id = value_get_id(key);
  size_t index = vec_get_size(self->keys);
  vec_push(self->keys, key);
  vec_push(self->values, value);

  if (self->index_type == MAP_INDEX_HASH) {
    size_t bucket_idx = hash_bucket_index(self, key_id);
    map_entry_index_t *new_entry =
        create_entry_index(self->allocator, key_id, index);
    list_push(self->index.buckets[bucket_idx], new_entry);

    if (vec_get_size(self->keys) >= DEFAULT_BUCKET_COUNT) {
      convert_to_rbtree(self);
    }
  } else {
    map_entry_index_t *new_entry =
        create_entry_index(self->allocator, key_id, index);
    rbtree_insert(self->index.rbtree, key_id, new_entry);
  }

  return vec_get_size(self->keys);
}

static bool remove_entry_from_bucket(list_t bucket, uint64_t key_id) {
  list_iter_t iter = list_iter_first(bucket);
  map_entry_index_t *entry;
  while ((entry = (map_entry_index_t *)list_iter_get(&iter)) != NULL) {
    if (entry->key_id == key_id) {
      list_iter_remove(&iter);
      return true;
    }
    list_iter_next(&iter);
  }
  return false;
}

size_t map_remove(map_t self, void *key) {
  map_entry_index_t *entry = find_entry_by_key(self, key);
  if (entry == NULL) {
    return vec_get_size(self->keys);
  }

  size_t idx = entry->index;

  if (self->index_type == MAP_INDEX_HASH) {
    /* Update indices of entries after the removed one before removing from
       index structure (entry is still accessible until removed from bucket) */
    for (size_t b = 0; b < DEFAULT_BUCKET_COUNT; b++) {
      list_iter_t iter = list_iter_first(self->index.buckets[b]);
      map_entry_index_t *e;
      while ((e = (map_entry_index_t *)list_iter_next(&iter)) != NULL) {
        if (e->index > idx) {
          e->index--;
        }
      }
    }
    size_t bucket_idx = hash_bucket_index(self, entry->key_id);
    list_t bucket = self->index.buckets[bucket_idx];
    remove_entry_from_bucket(bucket, entry->key_id);
  } else {
    /* Update indices of entries in rbtree */
    rbtree_iter_t iter = rbtree_iter_first(self->index.rbtree);
    map_entry_index_t *e;
    while ((e = (map_entry_index_t *)rbtree_iter_next(&iter)) != NULL) {
      if (e->index > idx) {
        e->index--;
      }
    }
    rbtree_remove(self->index.rbtree, entry->key_id);
  }

  vec_remove(self->keys, idx);
  vec_remove(self->values, idx);

  return vec_get_size(self->keys);
}

size_t map_clear(map_t self) {
  vec_resize(self->keys, 0);
  vec_resize(self->values, 0);
  if (self->index_type == MAP_INDEX_HASH) {
    for (size_t i = 0; i < DEFAULT_BUCKET_COUNT; i++) {
      list_clear(self->index.buckets[i]);
    }
  } else {
    rbtree_clear(self->index.rbtree);
  }
  return 0;
}

map_iter_t map_iter_first(map_t map) {
  map_iter_t iter = {
      .map = map,
      .current_idx = 0,
  };
  return iter;
}

void *map_iter_next(map_iter_t *iter) {
  if (iter->current_idx >= vec_get_size(iter->map->keys)) {
    return NULL;
  }
  void *value = vec_get(iter->map->values, iter->current_idx);
  iter->current_idx++;
  return value;
}