#ifndef _H_CUBEC_CORE_ARRAY_
#define _H_CUBEC_CORE_ARRAY_
#include "core/allocator.h"
#include "core/compare.h"
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_array_t *cubec_array_t;
typedef struct {
  size_t capacity;
  bool autofree;
  cubec_compare_fn_t compare;
} cubec_array_initialize_t;

cubec_array_t cubec_create_array(cubec_allocator_t allocator,
                                 cubec_array_initialize_t *initialize);
size_t cubec_array_get_size(const cubec_array_t self);
size_t cubec_array_get_capacity(const cubec_array_t self);
cubec_array_t cubec_array_shrink_to_fit(cubec_array_t self,
                                        cubec_allocator_t allocator);
cubec_array_t cubec_array_resize(cubec_array_t self,
                                 cubec_allocator_t allocator, size_t size);
void *cubec_array_get_index(const cubec_array_t self, size_t index);
void cubec_array_set_index(cubec_array_t self, cubec_allocator_t allocator,
                           size_t index, void *data);
void cubec_array_push(cubec_array_t self, cubec_allocator_t allocator,
                      void *data);
void cubec_array_pop(cubec_array_t self, cubec_allocator_t allocator);
void *cubec_array_back(cubec_array_t self, cubec_allocator_t allocator);
void cubec_array_swap(cubec_array_t self, size_t origin, size_t target);
void cubec_array_clear(cubec_array_t self, cubec_allocator_t allocator);
void cubec_array_sort(cubec_array_t self, void *cmp_arg);
cubec_array_t cubec_array_clone(cubec_allocator_t allocator,
                                const cubec_array_t src);
size_t cubec_array_find_index(cubec_array_t self, const void *value,
                              void *cmp_arg);
void *cubec_array_get_data(cubec_array_t self);
#ifdef __cplusplus
}
#endif
#endif