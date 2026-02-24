#ifndef _H_CUBEC_CORE_RBTREE_
#define _H_CUBEC_CORE_RBTREE_
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
#include "core/allocator.h"
#include "core/compare.h"
typedef struct _cubec_rbtree_node_t *cubec_rbtree_node_t;
typedef struct _cubec_rbtree_t *cubec_rbtree_t;
typedef struct _cubec_rbtree_initialize_t {
  bool autofree;
  cubec_compare_fn_t compare;
} cubec_rbtree_initialize_t;
cubec_rbtree_t cubec_create_rbtree(cubec_allocator_t allocator,
                                   cubec_rbtree_initialize_t *initialize);
void cubec_rbtree_put(cubec_rbtree_t self, void *key, void *cmp_arg);
bool cubec_rbtree_has(cubec_rbtree_t self, const void *key, void *cmp_arg);
void *cubec_rbtree_get(cubec_rbtree_t self, const void *key, void *cmp_arg);
void cubec_rbtree_remove(cubec_rbtree_t self, const void *key, void *cmp_arg);
size_t cubec_rbtree_size(cubec_rbtree_t self);
cubec_rbtree_node_t cubec_rbtree_get_first(cubec_rbtree_t self);
cubec_rbtree_node_t cubec_rbtree_get_last(cubec_rbtree_t self);
cubec_rbtree_node_t cubec_rbtree_node_next(cubec_rbtree_node_t self);
cubec_rbtree_node_t cubec_rbtree_node_last(cubec_rbtree_node_t self);
void *cubec_rbtree_node_get(cubec_rbtree_node_t self);
#ifdef __cplusplus
};
#endif
#endif