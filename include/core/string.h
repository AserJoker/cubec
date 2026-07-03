#ifndef _H_CUBEC_CORE_STRING_
#define _H_CUBEC_CORE_STRING_
#ifdef __cplusplus
extern "C" {
#endif
#include "core/type.h"
#include <stdint.h>
struct _string_t;
typedef struct _string_t *string_t;
typedef struct _string_init_t string_init_t;
struct _string_init_t {
  const char *str;
};
extern type_t g_string_type;
const char *string_get(string_t self);
size_t string_set(string_t self, const char *str);
size_t string_get_length(string_t self);
size_t string_concat(string_t self, const char *another);
size_t string_nconcat(string_t self, const char *another, size_t len);
#ifdef __cplusplus
}
#endif
#endif