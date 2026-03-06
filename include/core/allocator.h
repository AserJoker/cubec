#ifndef _H_CUBEC_CORE_ALLOCATOR_
#define _H_CUBEC_CORE_ALLOCATOR_
#ifdef __cplusplus
extern "C" {
#endif
#include <stdlib.h>
typedef struct _cubec_allocator_t *cubec_allocator_t;
typedef void (*cubec_dispose_fn_t)(void *self, cubec_allocator_t allocator);

typedef struct {
  void *(*alloc)(size_t len);
  void (*free)(void *data);
} cubec_allocator_initialize_t;

cubec_allocator_t
cubec_create_allocator(cubec_allocator_initialize_t *initialize);
void cubec_delete_allocator(cubec_allocator_t allocator);

void *cubec_allocator_alloc_debug(cubec_allocator_t self, size_t len,
                                  cubec_dispose_fn_t dispose,
                                  const char *filename, size_t line);
#define cubec_allocator_alloc(self, len, dispose)                              \
  cubec_allocator_alloc_debug(self, len, dispose, __FILE__, __LINE__)
void cubec_allocator_free(cubec_allocator_t self, void *data);
#ifdef __cplusplus
}
#endif
#endif