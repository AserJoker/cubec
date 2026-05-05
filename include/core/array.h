#ifndef _H_CORE_ARRAY_
#define _H_CORE_ARRAY_
#include "core/allocator.h"
#include "core/compare.h"
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _array_t *array_t;
typedef struct {
  size_t capacity;
  bool autofree;
  compare_fn_t compare;
} array_initialize_t;

array_t create_array(allocator_t allocator, array_initialize_t *initialize);
size_t array_get_size(const array_t self);
size_t array_get_capacity(const array_t self);
array_t array_shrink_to_fit(array_t self);
array_t array_resize(array_t self, size_t size);
void *array_get(const array_t self, size_t index);
void array_del(array_t self, size_t index);
void array_set(array_t self, size_t index, void *data);
void array_insert(array_t self, size_t index, void *data);
void *array_move(array_t self, size_t index);
void *array_replace(array_t self, size_t index, void *data);
void array_push(array_t self, void *data);
void array_pop(array_t self);
void *array_back(array_t self);
void array_swap(array_t self, size_t origin, size_t target);
void array_clear(array_t self);
void array_sort(array_t self, void *cmp_arg);
array_t array_clone(allocator_t allocator, const array_t src);
size_t array_find_index(array_t self, const void *value, void *cmp_arg);
void *array_get_data(array_t self);
#ifdef __cplusplus
}
#endif
#endif