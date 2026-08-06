#ifndef _H_CUBEC_ENGINE_STYPE_
#define _H_CUBEC_ENGINE_STYPE_

#include "core/allocator.h"
#include "core/node.h"
#include "core/vec.h"
#include "engine/def.h"
#ifdef __cplusplus
extern "C" {
#endif

/** @brief Specific kind within a stype_t object. */
enum type_kind_t {
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
  enum type_kind_t type_kind;
  vec_t params;     /* generic params (nullable, NULL for non-generic) */
  vec_t implements; /* type-erased instance array (nullable) */
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

/** @brief Dispose a stype_t and its owned sub-objects. */
void stype_dispose(stype_t type);

#ifdef __cplusplus
}
#endif

#endif /* _H_CUBEC_ENGINE_STYPE_ */
