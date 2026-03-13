#ifndef _H_CUBEC_CORE_STRING_
#define _H_CUBEC_CORE_STRING_
#include "core/allocator.h"
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _cubec_string_t *cubec_string_t;
typedef struct {
  const char *source;
} cubec_string_initialize_t;

cubec_string_t cubec_create_string(cubec_allocator_t allocator,
                                   cubec_string_initialize_t *initialize);
const char *cubec_string_get(cubec_string_t self);

size_t cubec_string_len(cubec_string_t self);

cubec_string_t cubec_string_set(cubec_string_t self,
                                cubec_allocator_t allocator,
                                const char *source);
cubec_string_t cubec_string_concat(cubec_string_t self,
                                   cubec_allocator_t allocator,
                                   const char *source);
int cubec_string_compare(cubec_string_t self, const char *source);

char *cubec_create_cstring(cubec_allocator_t allocator, const char *source);

const char *cubec_cstring_to_int(const char *source, size_t *value, int radix);
const char *cubec_cstring_to_dec(const char *source, double *value);

#ifdef __cplusplus
}
#endif
#endif