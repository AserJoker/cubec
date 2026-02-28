#ifndef _H_CUBEC_ENGINE_VARIABLE_
#define _H_CUBEC_ENGINE_VARIABLE_
#include "core/allocator.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_variable_t *cubec_variable_t;
struct _cubec_variable_t {
  cubec_value_t value;
  bool mutable;
};
cubec_variable_t cubec_create_varaible(cubec_allocator_t allocator,
                                       cubec_value_t value, bool mutable);
#ifdef __cplusplus
}
#endif
#endif