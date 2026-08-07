#ifndef _H_CUBEC_ENGINE_CUNION_INSTANCE_
#define _H_CUBEC_ENGINE_CUNION_INSTANCE_

#include "core/allocator.h"
#include "core/vec.h"
#include "engine/stype.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief C-union instance — a concrete instantiation of a cunion stype_t.
 *
 * C-unions are C ABI compatible and cannot be generic.
 * Always has a single instance (hash=0 in stype_t.implements).
 *
 * instance.name is owned.
 * field_names is owned (vec of owned char* strings).
 * field_types is borrowing (vec of stype_t pointers into context->types).
 */
struct _cunion_instance_t {
  stype_instance_header_t instance;  /* embedded header: name, hash, size, align */
  allocator_t allocator;
  vec_t field_names;  /* owned: vec of char* field names */
  vec_t field_types;  /* borrowing: vec of stype_t field types */
};

typedef struct _cunion_instance_t *cunion_instance_t;

/**
 * @brief Create a cunion_instance_t.
 * @param allocator    Allocator for this object
 * @param name         Instance name (copied, owned by instance)
 * @param hash         Structural hash for this instance
 * @param size         Byte size of this instance
 * @param align        Alignment of this instance
 * @param field_names  vec of char* field names (ownership transferred)
 * @param field_types  vec of stype_t field types (borrowing, may be NULL)
 */
cunion_instance_t cunion_instance_create(allocator_t allocator,
                                         const char *name,
                                         uint64_t hash,
                                         uint64_t size,
                                         uint64_t align,
                                         vec_t field_names,
                                         vec_t field_types);

/** @brief Dispose a cunion_instance_t and its owned sub-objects. */
void cunion_instance_dispose(cunion_instance_t inst);

#ifdef __cplusplus
}
#endif
#endif /* _H_CUBEC_ENGINE_CUNION_INSTANCE_ */
