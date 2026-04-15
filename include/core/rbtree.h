#ifndef _H_CUBEC_CORE_RBTREE_
#define _H_CUBEC_CORE_RBTREE_
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
#include "core/allocator.h"
#include "core/compare.h"
typedef struct _rbtree_node_t *rbtree_node_t;
typedef struct _rbtree_t *rbtree_t;
typedef struct _rbtree_initialize_t {
  bool autofree;
  compare_fn_t compare;
} rbtree_initialize_t;
rbtree_t create_rbtree(allocator_t allocator, rbtree_initialize_t *initialize);
void rbtree_put(rbtree_t self, void *key, void *cmp_arg);
bool rbtree_has(rbtree_t self, const void *key, void *cmp_arg);
void *rbtree_get(rbtree_t self, const void *key, void *cmp_arg);
void rbtree_remove(rbtree_t self, const void *key, void *cmp_arg);
size_t rbtree_size(rbtree_t self);
rbtree_node_t rbtree_get_first(rbtree_t self);
rbtree_node_t rbtree_get_last(rbtree_t self);
rbtree_node_t rbtree_node_next(rbtree_node_t self);
rbtree_node_t rbtree_node_last(rbtree_node_t self);
void *rbtree_node_get(rbtree_node_t self);
#ifdef __cplusplus
};
#endif
#endif