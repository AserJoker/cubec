#include "core/strmap.h"
#include "core/allocator.h"
#include "core/string.h"
#include "core/vec.h"
#include "core/list.h"
#include "core/rbtree.h"
#include <string.h>

#define DEFAULT_BUCKET_COUNT 16

/* ===== index entry: links hash -> linear index ===== */

typedef struct _strmap_entry_index_t {
  uint64_t hash;
  size_t index;
} strmap_entry_index_t;

/* ===== strmap struct ===== */

struct _strmap_t {
  allocator_t allocator;
  bool value_auto_dispose;
  vec_t keys;       /* vec of string_t (key objects) */
  vec_t values;     /* vec of void* (value objects) */
  strmap_index_type_t index_type;
  union {
    list_t buckets[DEFAULT_BUCKET_COUNT];
    rbtree_t rbtree;
  } index;
};

/* ===== FNV-1a hash ===== */

static uint64_t strmap_hash(const char *key) {
  uint64_t h = 14695981039346656037ULL;
  while (*key) {
    h ^= (uint64_t)(unsigned char)*key++;
    h *= 1099511628211ULL;
  }
  return h;
}

/* ===== lifecycle ===== */

static void _strmap_init(strmap_t self, allocator_t allocator,
                         strmap_init_t *init) {
  if (init) {
    self->value_auto_dispose = init->value_auto_dispose;
  } else {
    self->value_auto_dispose = false;
  }
  self->allocator = allocator;
  vec_init_t keys_init = {.auto_dispose = true};
  vec_init_t values_init = {.auto_dispose = self->value_auto_dispose};
  self->keys = allocator_create(allocator, &g_vec_class, &keys_init);
  self->values = allocator_create(allocator, &g_vec_class, &values_init);
  self->index_type = STRMAP_INDEX_HASH;
  for (size_t i = 0; i < DEFAULT_BUCKET_COUNT; i++) {
    list_init_t list_init = {.auto_dispose = true};
    self->index.buckets[i] =
        allocator_create(allocator, &g_list_class, &list_init);
  }
}

static void _strmap_dispose(strmap_t self, allocator_t allocator) {
  strmap_clear(self);
  if (self->index_type == STRMAP_INDEX_HASH) {
    for (size_t i = 0; i < DEFAULT_BUCKET_COUNT; i++) {
      allocator_free(self->allocator, &self->index.buckets[i]);
    }
  } else {
    allocator_free(self->allocator, &self->index.rbtree);
  }
  allocator_free(allocator, &self->keys);
  allocator_free(allocator, &self->values);
}

#define STRMAP_NOT_FOUND ((size_t)-1)

static size_t find_entry_in_list(list_t list, uint64_t hash, strmap_t self,
                                 const char *key) {
  list_iter_t iter = list_iter_first(list);
  strmap_entry_index_t *entry;
  while ((entry = list_iter_next(&iter)) != NULL) {
    if (entry->hash == hash) {
      string_t stored = vec_get(self->keys, entry->index);
      if (strcmp(string_get(stored), key) == 0) {
        return entry->index;
      }
    }
  }
  return STRMAP_NOT_FOUND;
}

static size_t find_entry(strmap_t self, const char *key, uint64_t hash) {
  if (self->index_type == STRMAP_INDEX_HASH) {
    size_t bucket_idx = hash % DEFAULT_BUCKET_COUNT;
    return find_entry_in_list(self->index.buckets[bucket_idx], hash, self, key);
  } else {
    strmap_entry_index_t *entry =
        (strmap_entry_index_t *)rbtree_find(self->index.rbtree, hash);
    if (entry == NULL) {
      /* hash not found in rbtree; linear scan as fallback */
      size_t size = vec_get_size(self->keys);
      for (size_t i = 0; i < size; i++) {
        string_t s = vec_get(self->keys, i);
        if (strcmp(string_get(s), key) == 0) {
          return i;
        }
      }
      return STRMAP_NOT_FOUND;
    }
    string_t stored = vec_get(self->keys, entry->index);
    if (strcmp(string_get(stored), key) == 0) {
      return entry->index;
    }
    /* hash collision: linear scan for matching key */
    size_t size = vec_get_size(self->keys);
    for (size_t i = 0; i < size; i++) {
      if (i == entry->index) continue;
      string_t s = vec_get(self->keys, i);
      uint64_t s_hash = strmap_hash(string_get(s));
      if (s_hash == hash && strcmp(string_get(s), key) == 0) {
        return i;
      }
    }
    return STRMAP_NOT_FOUND;
  }
}

static void _strmap_clone(strmap_t self, allocator_t allocator,
                          strmap_t another) {
  self->value_auto_dispose = another->value_auto_dispose;
  self->allocator = allocator;
  vec_init_t keys_init = {.auto_dispose = true};
  vec_init_t values_init = {.auto_dispose = another->value_auto_dispose};
  self->keys = allocator_create(allocator, &g_vec_class, &keys_init);
  self->values = allocator_create(allocator, &g_vec_class, &values_init);
  self->index_type = STRMAP_INDEX_HASH;
  for (size_t i = 0; i < DEFAULT_BUCKET_COUNT; i++) {
    list_init_t list_init = {.auto_dispose = true};
    self->index.buckets[i] =
        allocator_create(allocator, &g_list_class, &list_init);
  }

  size_t size = vec_get_size(another->keys);
  for (size_t i = 0; i < size; i++) {
    string_t key = vec_get(another->keys, i);
    void *value = vec_get(another->values, i);
    void *cloned_value = alloc_clone(allocator, value);
    strmap_insert(self, string_get(key), cloned_value);
  }
}

static void _strmap_move(strmap_t self, allocator_t allocator,
                         strmap_t another) {
  self->value_auto_dispose = another->value_auto_dispose;
  self->allocator = allocator;
  self->keys = another->keys;
  self->values = another->values;
  self->index_type = another->index_type;
  self->index = another->index;

  another->keys =
      allocator_create(allocator, &g_vec_class, &(vec_init_t){false});
  another->values =
      allocator_create(allocator, &g_vec_class, &(vec_init_t){false});
  another->index_type = STRMAP_INDEX_HASH;
  for (size_t i = 0; i < DEFAULT_BUCKET_COUNT; i++) {
    list_init_t list_init = {.auto_dispose = true};
    another->index.buckets[i] =
        allocator_create(allocator, &g_list_class, &list_init);
  }
}

class_t g_strmap_class = {
    .size = sizeof(struct _strmap_t),
    .name = "cubec.core.strmap",
    .init = (class_init_fn_t)_strmap_init,
    .dispose = (class_dispose_fn_t)_strmap_dispose,
    .clone = (class_clone_fn_t)_strmap_clone,
    .move = (class_move_fn_t)_strmap_move,
};

/* ===== index entry creation ===== */

static strmap_entry_index_t *create_entry_index(allocator_t allocator,
                                                 uint64_t hash, size_t index) {
  strmap_entry_index_t *entry = (strmap_entry_index_t *)allocator_alloc(
      allocator, sizeof(strmap_entry_index_t));
  entry->hash = hash;
  entry->index = index;
  return entry;
}

/* ===== convert to rbtree when large ===== */

static void convert_to_rbtree(strmap_t self) {
  rbtree_init_t init = {.auto_dispose = true};

  list_t buckets[DEFAULT_BUCKET_COUNT];
  for (size_t i = 0; i < DEFAULT_BUCKET_COUNT; i++) {
    buckets[i] = self->index.buckets[i];
  }

  self->index.rbtree = allocator_create(self->allocator, &g_rbtree_class, &init);

  for (size_t i = 0; i < DEFAULT_BUCKET_COUNT; i++) {
    list_t bucket = buckets[i];
    list_iter_t iter = list_iter_first(bucket);
    strmap_entry_index_t *entry;
    while ((entry = list_iter_next(&iter)) != NULL) {
      if (rbtree_find(self->index.rbtree, entry->hash) == NULL) {
        strmap_entry_index_t *moved_entry =
            (strmap_entry_index_t *)alloc_move(self->allocator, entry);
        rbtree_insert(self->index.rbtree, moved_entry->hash, moved_entry);
      }
    }
  }

  for (size_t i = 0; i < DEFAULT_BUCKET_COUNT; i++) {
    allocator_free(self->allocator, &buckets[i]);
  }

  self->index_type = STRMAP_INDEX_RBTREE;
}

/* ===== public API ===== */

size_t strmap_get_size(strmap_t self) { return vec_get_size(self->keys); }

void *strmap_find(strmap_t self, const char *key) {
  uint64_t hash = strmap_hash(key);
  size_t idx = find_entry(self, key, hash);
  if (idx == STRMAP_NOT_FOUND) {
    return NULL;
  }
  return vec_get(self->values, idx);
}

void *strmap_insert(strmap_t self, const char *key, void *value) {
  uint64_t hash = strmap_hash(key);
  size_t idx = find_entry(self, key, hash);
  if (idx != STRMAP_NOT_FOUND) {
    void *old_value = vec_get(self->values, idx);
    vec_set(self->values, idx, value);
    return old_value;
  }

  /* create string_t key object */
  string_init_t key_init = {.str = key};
  string_t key_obj =
      allocator_create(self->allocator, &g_string_class, &key_init);

  size_t index = vec_get_size(self->keys);
  vec_push(self->keys, key_obj);
  vec_push(self->values, value);

  if (self->index_type == STRMAP_INDEX_HASH) {
    size_t bucket_idx = hash % DEFAULT_BUCKET_COUNT;
    strmap_entry_index_t *new_entry =
        create_entry_index(self->allocator, hash, index);
    list_push(self->index.buckets[bucket_idx], new_entry);

    if (vec_get_size(self->keys) >= DEFAULT_BUCKET_COUNT) {
      convert_to_rbtree(self);
    }
  } else {
    if (rbtree_find(self->index.rbtree, hash) == NULL) {
      strmap_entry_index_t *new_entry =
          create_entry_index(self->allocator, hash, index);
      rbtree_insert(self->index.rbtree, hash, new_entry);
    }
  }

  return NULL;
}

void *strmap_remove(strmap_t self, const char *key) {
  uint64_t hash = strmap_hash(key);
  size_t idx = find_entry(self, key, hash);
  if (idx == STRMAP_NOT_FOUND) {
    return NULL;
  }

  void *removed_value = vec_get(self->values, idx);

  if (self->index_type == STRMAP_INDEX_HASH) {
    size_t bucket_idx = hash % DEFAULT_BUCKET_COUNT;
    list_t bucket = self->index.buckets[bucket_idx];
    list_iter_t iter = list_iter_first(bucket);
    while (list_iter_get(&iter) != NULL) {
      strmap_entry_index_t *e = (strmap_entry_index_t *)list_iter_get(&iter);
      if (e->hash == hash && e->index == idx) {
        list_iter_remove(&iter);
        break;
      }
      list_iter_next(&iter);
    }
  } else {
    strmap_entry_index_t *rbtree_entry =
        (strmap_entry_index_t *)rbtree_find(self->index.rbtree, hash);
    if (rbtree_entry && rbtree_entry->index == idx) {
      rbtree_remove(self->index.rbtree, hash);
    }
  }

  /* update index of the swapped element */
  size_t last_idx = vec_get_size(self->keys) - 1;
  if (idx != last_idx) {
    string_t moved_key = vec_get(self->keys, last_idx);
    uint64_t moved_hash = strmap_hash(string_get(moved_key));

    if (self->index_type == STRMAP_INDEX_HASH) {
      size_t moved_bucket = moved_hash % DEFAULT_BUCKET_COUNT;
      list_iter_t iter = list_iter_first(self->index.buckets[moved_bucket]);
      while (list_iter_get(&iter) != NULL) {
        strmap_entry_index_t *e = (strmap_entry_index_t *)list_iter_get(&iter);
        if (e->hash == moved_hash && e->index == last_idx) {
          e->index = idx;
          break;
        }
        list_iter_next(&iter);
      }
    } else {
      strmap_entry_index_t *rbtree_entry =
          (strmap_entry_index_t *)rbtree_find(self->index.rbtree, moved_hash);
      if (rbtree_entry && rbtree_entry->index == last_idx) {
        rbtree_entry->index = idx;
      }
    }
  }

  vec_remove(self->keys, idx);
  vec_remove(self->values, idx);

  return removed_value;
}

void strmap_clear(strmap_t self) {
  vec_resize(self->keys, 0);
  vec_resize(self->values, 0);
  if (self->index_type == STRMAP_INDEX_HASH) {
    for (size_t i = 0; i < DEFAULT_BUCKET_COUNT; i++) {
      list_clear(self->index.buckets[i]);
    }
  } else {
    rbtree_clear(self->index.rbtree);
  }
}

/* ===== iterator ===== */

strmap_iter_t strmap_iter_first(strmap_t self) {
  strmap_iter_t iter;
  iter.map = self;
  iter.current_idx = 0;
  return iter;
}

const char *strmap_iter_next(strmap_iter_t *iter) {
  if (iter->current_idx >= vec_get_size(iter->map->keys)) {
    return NULL;
  }
  string_t key = vec_get(iter->map->keys, iter->current_idx);
  iter->current_idx++;
  return string_get(key);
}
