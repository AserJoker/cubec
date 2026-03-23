#ifndef _H_CUBEC_ENGINE_ENUM_
#define _H_CUBEC_ENGINE_ENUM_
#include "core/allocator.h"
#include "core/array.h"
#include "engine/type.h"
#include "engine/value.h"
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_enum_option_t *cubec_enum_option_t;
struct _cubec_enum_option_t {
  char *name;
  void *value;
};

typedef struct _cubec_enum_meta_t *cubec_enum_meta_t;
struct _cubec_enum_meta_t {
  cubec_type_t type;
  cubec_array_t options;
  char *name;
};
cubec_enum_meta_t cubec_create_enum_meta(cubec_allocator_t allocator,
                                         cubec_type_t type, const char *name);
void cubec_add_enum_option(cubec_type_t self, cubec_allocator_t allocator,
                           const char *name, cubec_value_t value);
#ifdef __cplusplus
}
#endif
#endif