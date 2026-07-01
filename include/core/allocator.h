#ifndef _H_CUBEC_CORE_ALLOCATOR_
#define _H_CUBEC_CORE_ALLOCATOR_
#include "core/type.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#include <stdlib.h>
struct _allocator_t;
typedef struct _allocator_t *allocator_t;
typedef void *(*alloc_fn_t)(size_t size);
typedef void (*free_fn_t)(void *);
typedef void (*dispose_fn_t)(allocator_t allocator, void *self);
allocator_t create_allocator(alloc_fn_t alloc_fn, free_fn_t free_fn);
void delete_allocator(allocator_t allocator);
void *allocator_alloc(allocator_t self, size_t len);
void *allocator_create(allocator_t self, type_t *type, void *arg);
void allocator_free(allocator_t self, void *data);

type_t *value_get_type(void *self);
uint64_t value_get_id(void *self);
void *value_clone(allocator_t allocator, void *another);
void *value_move(allocator_t allocator, void *another);
#ifdef __cplusplus
}
#endif
#endif