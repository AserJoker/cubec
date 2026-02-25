#ifndef _H_CUBEC_ENGINE_VALUE_
#define _H_CUBEC_ENGINE_VALUE_
#include "core/allocator.h"
#include "engine/type_kind.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_type_t *cubec_type_t;
struct _cubec_type_t {
  const char *name;
  cubec_type_kind_t kind;
};

cubec_type_t cubec_create_type(cubec_allocator_t allocator,
                               cubec_type_kind_t kind, const char *name,
                               size_t size);
typedef struct _cubec_value_t *cubec_value_t;
struct _cubec_value_t {
  cubec_type_t type;
};
#define declar_type(name, type)                                                \
  typedef struct _cubec_type_t *cubec_##name##_type_t;                         \
  cubec_##name##_type_t cubec_get_type_##name();                               \
  typedef struct _cubec_##name##_value_t *cubec_##name##_value_t;              \
  struct _cubec_##name##_value_t {                                             \
    struct _cubec_value_t super;                                               \
    type value;                                                                \
  };                                                                           \
  cubec_value_t cubec_create_##name##_value(cubec_allocator_t allocator,       \
                                            type value);

declar_type(int8, int8_t);
declar_type(int16, int16_t);
declar_type(int32, int32_t);
declar_type(int64, int64_t);
declar_type(uint8, uint8_t);
declar_type(uint16, uint16_t);
declar_type(uint32, uint32_t);
declar_type(uint64, uint64_t);
declar_type(float32, float);
declar_type(float64, double);
declar_type(str, const char *);
declar_type(boolean, bool);
declar_type(pointer, void *);

#ifdef __cplusplus
}
#endif
#endif