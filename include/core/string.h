#ifndef _H_CUBEC_CORE_STRING_
#define _H_CUBEC_CORE_STRING_
#include "core/allocator.h"
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _string_t *string_t;
typedef struct {
  const char *source;
} string_initialize_t;

string_t create_string(allocator_t allocator, string_initialize_t *initialize);
const char *string_get(string_t self);

size_t string_len(string_t self);

string_t string_set(string_t self, allocator_t allocator, const char *source);
string_t string_concat(string_t self, allocator_t allocator,
                       const char *source);
int string_compare(string_t self, const char *source);

char *create_cstring(allocator_t allocator, const char *source);

const char *cstring_to_int(const char *source, size_t *value, int radix);
const char *cstring_to_dec(const char *source, double *value);
int64_t cstring_sdb(const char *key);

#ifdef __cplusplus
}
#endif
#endif