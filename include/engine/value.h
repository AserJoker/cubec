#ifndef _H_CUBEC_ENGINE_VALUE_
#define _H_CUBEC_ENGINE_VALUE_
#include "core/allocator.h"
#include "engine/type.h"
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_value_t *cubec_value_t;
struct _cubec_value_t {
  cubec_type_t type;
  bool is_mutable;
  void *data;
};
cubec_value_t cubec_create_value(cubec_allocator_t allocator, cubec_type_t type,
                                 bool is_mutable, const void *data);
cubec_value_t cubec_create_comptime_value(cubec_allocator_t allocator,
                                          cubec_type_t type, bool is_mutable);
#ifdef __cplusplus
}
#endif
#endif