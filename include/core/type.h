#ifndef _H_CUBEC_CORE_TYPE_
#define _H_CUBEC_CORE_TYPE_
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
struct _allocator_t;
typedef struct _type_t type_t;

typedef void (*type_init_fn_t)(void *self, struct _allocator_t *allocator,
                               void *arg);
typedef void (*type_dispose_fn_t)(void *self, struct _allocator_t *allocator);
typedef void (*type_clone_fn_t)(void *self, struct _allocator_t *allocator,
                                void *another);
typedef void (*type_move_fn_t)(void *self, struct _allocator_t *allocator,
                               void *another);
struct _type_t {
  const size_t size;
  const char *name;
  type_init_fn_t init;
  type_dispose_fn_t dispose;
  type_clone_fn_t clone;
  type_move_fn_t move;
};
#ifdef __cplusplus
}
#endif
#endif