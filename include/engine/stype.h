#ifndef _H_CUBEC_ENGINE_STYPE_
#define _H_CUBEC_ENGINE_STYPE_

#include "core/allocator.h"
#include "core/node.h"
#include "core/rbtree.h"
#include "core/vec.h"
#include "engine/def.h"
#ifdef __cplusplus
extern "C" {
#endif

/** @brief Specific kind within a stype_t object. */
enum type_kind_t {
  /* Primitive types */
  TYPE_VOID,        /**< void — no value */
  TYPE_BOOL,        /**< bool */
  TYPE_I8,          /**< signed 8-bit integer */
  TYPE_I16,         /**< signed 16-bit integer */
  TYPE_I32,         /**< signed 32-bit integer */
  TYPE_I64,         /**< signed 64-bit integer */
  TYPE_U8,          /**< unsigned 8-bit integer */
  TYPE_U16,         /**< unsigned 16-bit integer */
  TYPE_U32,         /**< unsigned 32-bit integer */
  TYPE_U64,         /**< unsigned 64-bit integer */
  TYPE_F16,         /**< IEEE 754 binary16 half-precision */
  TYPE_F32,         /**< IEEE 754 binary32 single-precision */
  TYPE_F64,         /**< IEEE 754 binary64 double-precision */
  TYPE_CHAR,        /**< char — single Unicode code point */
  TYPE_STR,         /**< str — comptime string slice */
  TYPE_NIL,         /**< nil — typed null pointer */
  /* Named declaration types (created during def_collection phase 1) */
  TYPE_STRUCT,       /**< struct declaration */
  TYPE_UNION,        /**< union declaration */
  TYPE_ENUM,         /**< enum declaration */
  TYPE_INTERFACE,    /**< interface declaration */
  TYPE_TYPE_ALIAS,   /**< type alias declaration */
  TYPE_CUNION,       /**< C-style union declaration */
  /* Composite types (created during type resolution) */
  TYPE_POINTER,      /**< pointer type */
  TYPE_ARRAY,        /**< fixed-size array type */
  TYPE_SLICE,        /**< slice type */
  TYPE_TUPLE,        /**< tuple type */
  TYPE_CALLABLE      /**< function type expression */
};

/**
 * @brief Common instance header for all type instances.
 *
 * Embedded as first field in stype_t for primitive types (no extra fields),
 * and as header in struct/enum/union/composite type instances.
 * name is owned (disposed with stype).
 *
 * hash is a structural hash used for duck-typing compatibility:
 * two types are compatible iff their hashes match (collision → recursive compare).
 * Primitive types: hash = type_kind value.
 * Named types: hash = FNV-1a over {kind, field_names, field_type_hashes, generic_param_hashes}.
 * Composite types: hash = FNV-1a over {kind, component_type_hashes}.
 */
struct stype_instance_header_t {
  char *name;          /**< owned: type name (e.g. "i32", "MyStruct") */
  uint64_t hash;       /**< structural hash for duck-typing compatibility */
  uint64_t size;       /**< byte size */
  uint64_t align;      /**< alignment requirement */
  enum type_kind_t kind; /**< actual type kind (may differ from stype->type_kind for type_alias instances) */
};
typedef struct stype_instance_header_t stype_instance_header_t;

/**
 * @brief Semantic type — represents a type definition (named or composite).
 *
 * All types (named declarations and composite types like pointer, array,
 * slice, tuple, callable) share this template structure. Non-generic types
 * have params=NULL and a single hash=0 instance in implements.
 *
 * Owned by context_t (global type registry). Borrowed by name_t.ref.
 */
struct _stype_t {
  def_t header;
  stype_instance_header_t instance;  /**< instance header: name, size, align */
  enum type_kind_t type_kind;
  vec_t params;     /* generic params (nullable, NULL for non-generic) */
  rbtree_t implements; /* hash(uint64_t) → instance (auto-dispose, nullable) */
};

typedef struct _stype_t *stype_t;

/**
 * @brief Create a stype_t with the given kind and AST node.
 * @param allocator  Allocator for this object
 * @param kind       Specific type kind (TYPE_STRUCT, TYPE_POINTER, etc.)
 * @param node       AST declaration node (borrowing reference)
 * @return New stype_t with params=NULL, implements=NULL
 */
stype_t stype_create(allocator_t allocator, enum type_kind_t kind, node_t node);

/**
 * @brief Create a stype_t for a primitive type with instance metadata.
 * @param allocator  Allocator for this object
 * @param kind       Primitive type kind (TYPE_I32, TYPE_BOOL, etc.)
 * @param name       Type name string (copied, owned by stype)
 * @param size       Byte size of the type
 * @param align      Alignment requirement
 * @return New stype_t with node=NULL, params=NULL, implements=NULL
 */
stype_t stype_create_primitive(allocator_t allocator, enum type_kind_t kind,
                               const char *name, uint64_t size, uint64_t align);

/** @brief Dispose a stype_t and its owned sub-objects. */
void stype_dispose(stype_t type);

/* --------------------------------------------------------------------------
 *  Structural hash computation (FNV-1a based)
 * -------------------------------------------------------------------------- */

/**
 * @brief Compute hash for a primitive type (just the kind).
 * Primitive hashes are deterministic: hash(TYPE_I32) is always the same.
 */
uint64_t stype_compute_primitive_hash(enum type_kind_t kind);

/**
 * @brief FNV-1a: mix a uint64_t value into an existing hash.
 * Useful for computing temporary hashes from node pointers, etc.
 */
uint64_t stype_hash_mix_u64(uint64_t hash, uint64_t val);

/**
 * @brief Compute hash for a named type (struct/union/enum/interface/cunion).
 *
 * Includes generic argument hashes so that different instantiations of
 * the same generic type produce different hashes:
 *   Test[i32, 1] ≠ Test[i32, 2]
 *
 * @param kind               Type kind (TYPE_STRUCT, TYPE_UNION, etc.)
 * @param generic_arg_hashes vec of uint64_t (as uintptr_t) generic arg hashes
 *                           (NULL for non-generic types)
 * @param field_names        vec of const char* field names (borrowed, may be NULL)
 * @param field_type_hashes  vec of uint64_t (as uintptr_t) field type hashes
 *                           (may be NULL)
 */
uint64_t stype_compute_named_type_hash(enum type_kind_t kind,
                                       vec_t generic_arg_hashes,
                                       vec_t field_names,
                                       vec_t field_type_hashes);

/**
 * @brief Compute hash for a composite type (pointer/array/slice/tuple/callable).
 * @param kind                  Composite type kind
 * @param component_type_hashes vec of uint64_t (as uintptr_t) component hashes
 */
uint64_t stype_compute_composite_hash(enum type_kind_t kind,
                                      vec_t component_type_hashes);

#ifdef __cplusplus
}
#endif

#endif /* _H_CUBEC_ENGINE_STYPE_ */
