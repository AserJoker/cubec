#ifndef _H_CUBEC_ENGINE_ENUM_INSTANCE_
#define _H_CUBEC_ENGINE_ENUM_INSTANCE_

#include "core/allocator.h"
#include "core/vec.h"
#include "engine/stype.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _enum_instance_t {
  stype_instance_header_t instance;
  allocator_t allocator;
  vec_t variant_names;    /* owned: vec of char* */
  vec_t variant_values;   /* owned: vec of value_t (nullable) */
  stype_t underlying_type; /* borrowing */
};

typedef struct _enum_instance_t *enum_instance_t;

enum_instance_t enum_instance_create(allocator_t allocator,
                                     const char *name,
                                     uint64_t hash,
                                     uint64_t size,
                                     uint64_t align,
                                     vec_t variant_names,
                                     vec_t variant_values,
                                     stype_t underlying_type);

void enum_instance_dispose(enum_instance_t inst);

/** @brief Hash an enum value — interprets data as the underlying integer. */
uint64_t enum_instance_hash_value(stype_t type, uint64_t type_hash, const void *data);

#ifdef __cplusplus
}
#endif
#endif /* _H_CUBEC_ENGINE_ENUM_INSTANCE_ */
