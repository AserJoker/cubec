#ifndef _H_CUBEC_ENGINE_VARIABLE_
#define _H_CUBEC_ENGINE_VARIABLE_
#include "core/allocator.h"
#include "core/value.h"
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_variable_t *cubec_variable_t;
struct _cubec_variable_t {
  cubec_value_t value;
  bool is_stack;
  bool is_const;
};
cubec_variable_t cubec_create_variable(cubec_allocator_t allocator,
                                       cubec_value_t value, bool is_stack,
                                       bool is_const);

#ifdef __cplusplus
}
#endif
#endif