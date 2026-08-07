#ifndef _H_CUBEC_ENGINE_UNION_INSTANCE_
#define _H_CUBEC_ENGINE_UNION_INSTANCE_

#include "core/allocator.h"
#include "core/strmap.h"
#include "core/vec.h"
#include "engine/comptime_value.h"
#include "engine/union_field.h"
#include "engine/stype.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _function_t;
typedef struct _function_t *function_t;

/**
 * @brief Union instance — a concrete instantiation of a union stype_t.
 *
 * fields is owned (vec of union_field_t, ordered by declaration).
 * members is owned (strmap: name → union_field_t, borrowing from fields).
 * methods is owned (strmap: name → function_t, borrowing).
 */
struct _union_instance_t {
  stype_instance_header_t instance;
  allocator_t allocator;
  vec_t fields;      /* owned: vec of union_field_t */
  strmap_t members;  /* owned: name → union_field_t (borrowing) */
  strmap_t methods;  /* owned: name → function_t (borrowing) */
};

typedef struct _union_instance_t *union_instance_t;

union_instance_t union_instance_create(allocator_t allocator,
                                       const char *name,
                                       uint64_t hash,
                                       uint64_t size,
                                       uint64_t align,
                                       vec_t fields,
                                       strmap_t members,
                                       strmap_t methods);

void union_instance_dispose(union_instance_t inst);

/* ---- Union comptime value operations ---- */

void union_instance_dispose_value(comptime_value_t val);
comptime_value_t union_instance_clone_value(allocator_t allocator, comptime_value_t val);
uint64_t union_instance_hash_value(comptime_value_t val);

#ifdef __cplusplus
}
#endif
#endif /* _H_CUBEC_ENGINE_UNION_INSTANCE_ */
