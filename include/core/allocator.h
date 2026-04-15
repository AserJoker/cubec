#ifndef _H_CUBEC_CORE_ALLOCATOR_
#define _H_CUBEC_CORE_ALLOCATOR_
#ifdef __cplusplus
extern "C" {
#endif
#include <stdlib.h>
typedef struct _allocator_t *allocator_t;
typedef void (*dispose_fn_t)(void *self, allocator_t allocator);

typedef struct {
  void *(*alloc)(size_t len);
  void (*free)(void *data);
} allocator_initialize_t;

allocator_t create_allocator(allocator_initialize_t *initialize);
void delete_allocator(allocator_t allocator);

void *allocator_alloc_debug(allocator_t self, size_t len, dispose_fn_t dispose,
                            const char *filename, size_t line);
#define allocator_alloc(self, len, dispose)                                    \
  allocator_alloc_debug(self, len, dispose, __FILE__, __LINE__)
void allocator_free(allocator_t self, void *data);
#ifdef __cplusplus
}
#endif
#endif