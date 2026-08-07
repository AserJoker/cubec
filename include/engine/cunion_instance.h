#ifndef _H_CUBEC_ENGINE_CUNION_INSTANCE_
#define _H_CUBEC_ENGINE_CUNION_INSTANCE_

#include "core/allocator.h"
#include "core/vec.h"
#include "engine/comptime_value.h"
#include "engine/union_field.h"
#include "engine/stype.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief C-union instance — a concrete instantiation of a cunion stype_t.
 *
 * C-unions are C ABI compatible and cannot be generic.
 * Only has fields (no member/method maps — C unions don't have those).
 */
struct _cunion_instance_t {
  stype_instance_header_t instance;
  allocator_t allocator;
  vec_t fields;  /* owned: vec of union_field_t */
};

typedef struct _cunion_instance_t *cunion_instance_t;

cunion_instance_t cunion_instance_create(allocator_t allocator,
                                         const char *name,
                                         uint64_t hash,
                                         uint64_t size,
                                         uint64_t align,
                                         vec_t fields);

void cunion_instance_dispose(cunion_instance_t inst);

/* ---- C-union comptime value operations ---- */

void cunion_instance_dispose_value(comptime_value_t val);
comptime_value_t cunion_instance_clone_value(allocator_t allocator, comptime_value_t val);
uint64_t cunion_instance_hash_value(comptime_value_t val);

#ifdef __cplusplus
}
#endif
#endif /* _H_CUBEC_ENGINE_CUNION_INSTANCE_ */
