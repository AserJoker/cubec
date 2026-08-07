#ifndef _H_CUBEC_ENGINE_UNION_INSTANCE_
#define _H_CUBEC_ENGINE_UNION_INSTANCE_

#include "core/allocator.h"
#include "core/vec.h"
#include "engine/stype.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Union instance — a concrete instantiation of a union stype_t.
 *
 * Non-generic unions have a single instance (hash=0 in stype_t.implements).
 * Generic unions have one instance per set of concrete type arguments.
 *
 * instance.name is owned.
 * field_names is owned (vec of owned char* strings).
 * field_types is borrowing (vec of stype_t pointers into context->types).
 */
struct _union_instance_t {
  stype_instance_header_t instance;  /* embedded header: name, hash, size, align */
  allocator_t allocator;
  vec_t field_names;  /* owned: vec of char* field names */
  vec_t field_types;  /* borrowing: vec of stype_t field types */
};

typedef struct _union_instance_t *union_instance_t;

/**
 * @brief Create a union_instance_t.
 * @param allocator    Allocator for this object
 * @param name         Instance name (copied, owned by instance)
 * @param hash         Structural hash for this instance
 * @param size         Byte size of this instance
 * @param align        Alignment of this instance
 * @param field_names  vec of char* field names (ownership transferred)
 * @param field_types  vec of stype_t field types (borrowing, may be NULL)
 */
union_instance_t union_instance_create(allocator_t allocator,
                                       const char *name,
                                       uint64_t hash,
                                       uint64_t size,
                                       uint64_t align,
                                       vec_t field_names,
                                       vec_t field_types);

/** @brief Dispose a union_instance_t and its owned sub-objects. */
void union_instance_dispose(union_instance_t inst);

#ifdef __cplusplus
}
#endif
#endif /* _H_CUBEC_ENGINE_UNION_INSTANCE_ */
