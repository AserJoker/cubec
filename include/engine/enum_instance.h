#ifndef _H_CUBEC_ENGINE_ENUM_INSTANCE_
#define _H_CUBEC_ENGINE_ENUM_INSTANCE_

#include "core/allocator.h"
#include "core/vec.h"
#include "engine/stype.h"
#include "engine/comptime_value.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enum instance — a concrete instantiation of an enum stype_t.
 *
 * Non-generic enums have a single instance (hash=0 in stype_t.implements).
 * Generic enums have one instance per set of concrete type arguments.
 *
 * instance.name is owned.
 * variant_names is owned (vec of owned char* strings).
 * variant_values is owned (vec of comptime_value_t, nullable for auto-increment).
 * underlying_type is borrowing (stype_t in context->types).
 */
struct _enum_instance_t {
  stype_instance_header_t instance;    /* embedded header: name, hash, size, align */
  allocator_t allocator;
  vec_t variant_names;    /* owned: vec of char* variant names */
  vec_t variant_values;   /* owned: vec of comptime_value_t (nullable for auto) */
  stype_t underlying_type; /* borrowing: backing integer type */
};

typedef struct _enum_instance_t *enum_instance_t;

/**
 * @brief Create an enum_instance_t.
 * @param allocator        Allocator for this object
 * @param name             Instance name (copied, owned by instance)
 * @param hash             Structural hash for this instance
 * @param size             Byte size of this instance
 * @param align            Alignment of this instance
 * @param variant_names    vec of char* variant names (ownership transferred)
 * @param variant_values   vec of comptime_value_t values (ownership transferred, may be NULL)
 * @param underlying_type  Backing integer type (borrowing)
 */
enum_instance_t enum_instance_create(allocator_t allocator,
                                     const char *name,
                                     uint64_t hash,
                                     uint64_t size,
                                     uint64_t align,
                                     vec_t variant_names,
                                     vec_t variant_values,
                                     stype_t underlying_type);

/** @brief Dispose an enum_instance_t and its owned sub-objects. */
void enum_instance_dispose(enum_instance_t inst);

#ifdef __cplusplus
}
#endif
#endif /* _H_CUBEC_ENGINE_ENUM_INSTANCE_ */
