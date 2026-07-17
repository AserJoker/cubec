#ifndef _H_CUBEC_ENGINE_SEMANTIC_TYPE_
#define _H_CUBEC_ENGINE_SEMANTIC_TYPE_
#include "core/location.h"
#include "core/type.h"
#include "core/vec.h"
#include <stdbool.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Semantic type kinds (implementation layer).
 */
enum type_kind {
  TYPE_VOID, TYPE_BOOL,
  TYPE_I8, TYPE_I16, TYPE_I32, TYPE_I64,
  TYPE_U8, TYPE_U16, TYPE_U32, TYPE_U64,
  TYPE_F16, TYPE_F32, TYPE_F64,
  TYPE_CHAR, TYPE_STRING,
  TYPE_POINTER, TYPE_SLICE, TYPE_ARRAY,
  TYPE_STRUCT, TYPE_UNION, TYPE_CUNION, TYPE_ENUM,
  TYPE_INTERFACE, TYPE_FUNCTION, TYPE_TYPE, TYPE_QUALIFIER,
  TYPE_GENERIC_INSTANCE,
  TYPE_NIL, TYPE_ERROR
};

/* forward declaration */
struct _type_impl;
typedef struct _type_impl *type_impl_t;

/**
 * @brief Name-layer entry: a named type with associated members.
 *        This is the public handle (semantic_type_t).
 */
struct type_name_entry {
  const char *name;       /**< Type name (may be NULL for anonymous) */
  type_impl_t impl;       /**< Implementation layer */
  vec_t instance_methods; /**< vec of symbol* (FUNCTION with self) */
  vec_t static_methods;   /**< vec of symbol* (FUNCTION without self) */
  vec_t static_fields;    /**< vec of symbol* (VARIABLE) */
  vec_t associated_types; /**< vec of symbol* (TYPE) */
  bool is_interface;      /**< true for interface types */
  bool is_incomplete;     /**< true until fields are resolved */
};

typedef struct type_name_entry *semantic_type_t;

/**
 * @brief Implementation layer: structural type information.
 */
struct _type_impl {
  size_t hash;              /**< Structural hash for dedup */
  enum type_kind kind;      /**< Type kind */
  size_t size;              /**< Size in bytes (0 until layout computed) */
  size_t alignment;         /**< Alignment in bytes (0 until layout computed) */
  bool is_packed;           /**< Packed struct/union */
  size_t explicit_align;    /**< Explicit alignment (0 = natural) */
  union {
    /* TYPE_QUALIFIER */
    struct { semantic_type_t base; bool is_const; bool is_volatile; } qualifier;
    /* TYPE_POINTER */
    struct { semantic_type_t pointee; } pointer;
    /* TYPE_SLICE */
    struct { semantic_type_t element; } slice;
    /* TYPE_ARRAY */
    struct { semantic_type_t element; size_t length; } array;
    /* TYPE_STRUCT / TYPE_UNION / TYPE_CUNION */
    struct { vec_t fields; } struct_type;  /* vec of symbol* (FIELD) */
    /* TYPE_ENUM */
    struct { vec_t items; semantic_type_t backing_type; } enum_type; /* vec of symbol* (ENUM_ITEM) */
    /* TYPE_INTERFACE */
    struct { vec_t methods; } interface_type; /* vec of symbol* (FUNCTION) */
    /* TYPE_FUNCTION */
    struct { semantic_type_t return_type; vec_t params; bool is_variadic; } function;
    /* TYPE_TYPE (type-of-type) */
    struct { semantic_type_t inner; } type_of;
    /* TYPE_GENERIC_INSTANCE */
    struct {
      semantic_type_t generic_template;  /**< Generic template type */
      vec_t type_args;                   /**< Concrete type arguments (semantic_type_t), auto_dispose=false */
    } generic_instance;
  };
};

/** @brief Virtual table for semantic_type_t. */
extern type_t g_semantic_type_type;

/* ===== query API ===== */

enum type_kind semantic_type_get_kind(semantic_type_t self);
const char *semantic_type_get_name(semantic_type_t self);
size_t semantic_type_get_size(semantic_type_t self);
size_t semantic_type_get_alignment(semantic_type_t self);
bool semantic_type_is_incomplete(semantic_type_t self);
type_impl_t semantic_type_get_impl(semantic_type_t self);

/**
 * @brief Structural equality check.
 *        Uses hash first, then recursive structural comparison.
 */
bool semantic_type_equals(semantic_type_t a, semantic_type_t b);

/**
 * @brief Check if `from` can decay to `to` (e.g., array -> slice, array -> pointer).
 */
bool semantic_type_can_decay(semantic_type_t from, semantic_type_t to);

/**
 * @brief Check if `from` can implicitly convert to `to`.
 */
bool semantic_type_can_implicit_convert(semantic_type_t from, semantic_type_t to);

/* ===== constructors ===== */

semantic_type_t semantic_type_create_named(allocator_t allocator,
                                           const char *name,
                                           enum type_kind kind);
semantic_type_t semantic_type_create_pointer(allocator_t allocator,
                                             semantic_type_t pointee);
semantic_type_t semantic_type_create_slice(allocator_t allocator,
                                           semantic_type_t element);
semantic_type_t semantic_type_create_array(allocator_t allocator,
                                           semantic_type_t element,
                                           size_t length);
semantic_type_t semantic_type_create_qualifier(allocator_t allocator,
                                               semantic_type_t base,
                                               bool is_const, bool is_volatile);

/* ===== qualifier query utilities ===== */

bool semantic_type_is_const(semantic_type_t type);
bool semantic_type_is_volatile(semantic_type_t type);
semantic_type_t semantic_type_strip_qualifier(semantic_type_t type);
semantic_type_t semantic_type_create_function(allocator_t allocator,
                                              semantic_type_t return_type,
                                              vec_t params,
                                              bool is_variadic);
semantic_type_t semantic_type_create_generic_instance(allocator_t allocator,
                                                       semantic_type_t template_type,
                                                       vec_t type_args);

#ifdef __cplusplus
}
#endif
#endif
