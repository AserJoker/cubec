#ifndef _H_CUBEC_CORE_VEC_
#define _H_CUBEC_CORE_VEC_
#include "core/type.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
extern type_t g_vec_type;
struct _vec_t;
typedef struct _vec_t *vec_t;

typedef struct _vec_init_t vec_init_t;
struct _vec_init_t {
  bool auto_dispose;
};
size_t vec_get_size(vec_t self);
size_t vec_get_capacity(vec_t self);
void **vec_get_data(vec_t self);
void *vec_get(vec_t self, size_t idx);
size_t vec_set(vec_t self, size_t idx, void *data);
size_t vec_resize(vec_t self, size_t size);
size_t vec_push(vec_t self, void *data);
size_t vec_pop(vec_t self);
size_t vec_remove(vec_t self, size_t idx);
size_t vec_insert(vec_t self, size_t idx, void *data);
#ifdef __cplusplus
}
#endif
#endif