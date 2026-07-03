#ifndef _H_CUBEC_CORE_LIST_
#define _H_CUBEC_CORE_LIST_
#include "core/type.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

struct _list_t;
typedef struct _list_t *list_t;

typedef struct _list_node_t list_node_t;
struct _list_node_t {
  list_node_t *prev;
  list_node_t *next;
  void *data;
};

typedef struct _list_init_t list_init_t;
struct _list_init_t {
  bool auto_dispose;
};

typedef struct _list_iter_t list_iter_t;
struct _list_iter_t {
  list_t list;
  void *current;
};

extern type_t g_list_type;

size_t list_get_size(list_t self);
void **list_get_data(list_t self);
void *list_get_first(list_t self);
void *list_get_last(list_t self);
void *list_get(list_t self, size_t idx);
size_t list_set(list_t self, size_t idx, void *data);
size_t list_push(list_t self, void *data);
void *list_pop(list_t self);
size_t list_unshift(list_t self, void *data);
void *list_shift(list_t self);
size_t list_insert(list_t self, size_t idx, void *data);
size_t list_remove(list_t self, size_t idx);
size_t list_clear(list_t self);
list_iter_t list_iter_first(list_t list);
void *list_iter_next(list_iter_t *iter);

#ifdef __cplusplus
}
#endif
#endif