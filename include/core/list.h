#ifndef _H_CORE_LIST_
#define _H_CORE_LIST_
#include "core/allocator.h"
#include "core/compare.h"
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
struct _list_t;
typedef struct _list_t *list_t;
struct _list_node_t;
typedef struct _list_node_t *list_node_t;
typedef struct list_initialize_t {
  bool autofree;
  compare_fn_t compare;
} list_initialize_t;

list_t create_list(allocator_t allocator, list_initialize_t *initialize);

list_node_t list_get_begin(list_t self);

list_node_t list_get_end(list_t self);

list_node_t list_get_first(list_t self);

list_node_t list_get_last(list_t self);

size_t list_get_size(list_t self);

void list_clear(list_t self);

void list_set_data(list_t self, list_node_t node, void *data);

void list_append(list_t self, void *data);

void list_insert(list_t self, list_node_t position, void *data);

void list_erase(list_t self, list_node_t position);

list_node_t list_find(list_t self, const void *data, void *cmp_arg);

list_node_t list_node_next(list_node_t self);

list_node_t list_node_last(list_node_t self);

void *list_node_get(list_node_t self);

void *list_node_move(list_node_t self);

#ifdef __cplusplus
}
#endif
#endif