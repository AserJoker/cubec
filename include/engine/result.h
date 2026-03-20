#ifndef _H_CUBEC_ENGINE_RESULT_
#define _H_CUBEC_ENGINE_RESULT_
#include "core/allocator.h"
#include "engine/type.h"
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_result_meta_t *cubec_result_meta_t;
struct _cubec_result_meta_t {
  cubec_type_t type;
  cubec_type_t error_type;
};
cubec_result_meta_t cubec_create_result_meta(cubec_allocator_t allocator,
                                             cubec_type_t type,
                                             cubec_type_t etype);
typedef struct _cubec_result_data_t *cubec_result_data_t;
struct _cubec_result_data_t {
  bool flag;
  uint8_t data[0];
};
#ifdef __cplusplus
}
#endif
#endif