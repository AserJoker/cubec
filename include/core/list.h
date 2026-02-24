#ifndef _H_CUBEC_CORE_LIST_
#define _H_CUBEC_CORE_LIST_
#include "core/allocator.h"
#include "core/compare.h"
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
struct _cubec_list_t;
typedef struct _cubec_list_t *cubec_list_t;
struct _cubec_list_node_t;
typedef struct _cubec_list_node_t *cubec_list_node_t;
typedef struct cubec_list_initialize_t {
  bool autofree;
  cubec_compare_fn_t compare;
} cubec_list_initialize_t;

cubec_list_t cubec_create_list(cubec_allocator_t allocator,
                               cubec_list_initialize_t *initialize);

cubec_list_node_t cubec_list_get_begin(cubec_list_t self);

cubec_list_node_t cubec_list_get_end(cubec_list_t self);

cubec_list_node_t cubec_list_get_first(cubec_list_t self);

cubec_list_node_t cubec_list_get_last(cubec_list_t self);

size_t cubec_list_get_size(cubec_list_t self);

void cubec_list_clear(cubec_list_t self, cubec_allocator_t allocator);

void cubec_list_set_data(cubec_list_t self, cubec_allocator_t allocator,
                         cubec_list_node_t node, void *data);

void cubec_list_append(cubec_list_t self, cubec_allocator_t allocator,
                       void *data);

void cubec_list_insert(cubec_list_t self, cubec_allocator_t allocator,
                       cubec_list_node_t position, void *data);

void cubec_list_erase(cubec_list_t self, cubec_allocator_t allocator,
                      cubec_list_node_t position);

cubec_list_node_t cubec_list_find(cubec_list_t self, const void *data,
                                  void *cmp_arg);

cubec_list_node_t cubec_list_node_next(cubec_list_node_t self);

cubec_list_node_t cubec_list_node_last(cubec_list_node_t self);

void *cubec_list_node_get(cubec_list_node_t self);

void *cubec_list_node_move(cubec_list_node_t self);

#ifdef __cplusplus
}
#endif
#endif