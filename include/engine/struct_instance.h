#ifndef _H_CUBEC_ENGINE_STRUCT_INSTANCE_
#define _H_CUBEC_ENGINE_STRUCT_INSTANCE_

#include "core/allocator.h"
#include "core/vec.h"
#include "engine/stype.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Struct instance — a concrete instantiation of a struct stype_t.
 *
 * Non-generic structs have a single instance (hash=0 in stype_t.implements).
 * Generic structs have one instance per set of concrete type arguments.
 *
 * instance.name is owned (e.g. "MyStruct" or "MyStruct<i32>").
 * field_names is owned (vec of owned char* strings).
 * field_types is borrowing (vec of stype_t pointers into context->types).
 * field_offsets is owned (vec of uint64_t byte offsets, computed during layout).
 */
struct _struct_instance_t {
  stype_instance_header_t instance;  /* embedded header: name, hash, size, align */
  allocator_t allocator;
  vec_t field_names;    /* owned: vec of char* field names */
  vec_t field_types;    /* borrowing: vec of stype_t field types */
  vec_t field_offsets;  /* owned: vec of uint64_t byte offsets */
};

typedef struct _struct_instance_t *struct_instance_t;

/**
 * @brief Create a struct_instance_t.
 * @param allocator      Allocator for this object
 * @param name           Instance name (copied, owned by instance)
 * @param hash           Structural hash for this instance
 * @param size           Byte size of this instance
 * @param align          Alignment of this instance
 * @param field_names    vec of char* field names (ownership transferred)
 * @param field_types    vec of stype_t field types (borrowing, may be NULL)
 * @param field_offsets  vec of uint64_t byte offsets (ownership transferred)
 */
struct_instance_t struct_instance_create(allocator_t allocator,
                                         const char *name,
                                         uint64_t hash,
                                         uint64_t size,
                                         uint64_t align,
                                         vec_t field_names,
                                         vec_t field_types,
                                         vec_t field_offsets);

/** @brief Dispose a struct_instance_t and its owned sub-objects. */
void struct_instance_dispose(struct_instance_t inst);

#ifdef __cplusplus
}
#endif
#endif /* _H_CUBEC_ENGINE_STRUCT_INSTANCE_ */
