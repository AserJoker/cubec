#ifndef _H_CUBEC_CORE_RBTREE_
#define _H_CUBEC_CORE_RBTREE_
#include "core/type.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _rbtree_t;
typedef struct _rbtree_t *rbtree_t;

typedef struct _rbtree_init_t rbtree_init_t;
struct _rbtree_init_t {
  bool auto_dispose;
};

typedef struct _rbtree_iter_t rbtree_iter_t;
struct _rbtree_iter_t {
  rbtree_t tree;
  void *current;
};

extern type_t g_rbtree_type;

size_t rbtree_get_size(rbtree_t self);
void *rbtree_find(rbtree_t self, uint64_t key);
void *rbtree_insert(rbtree_t self, uint64_t key, void *value);
size_t rbtree_remove(rbtree_t self, uint64_t key);
void rbtree_clear(rbtree_t self);
rbtree_iter_t rbtree_iter_first(rbtree_t tree);
void *rbtree_iter_next(rbtree_iter_t *iter);

#ifdef __cplusplus
}
#endif
#endif
