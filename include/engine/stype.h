#ifndef _H_CUBEC_ENGINE_STYPE_
#define _H_CUBEC_ENGINE_STYPE_
#include "engine/value.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Type kind — distinguishes value categories.
 *        Determined entirely by stype_t, no kind field on value_t.
 */
typedef enum type_kind_t {
  TYPE_KIND_TYPE,       /**< Type itself (stype_t as value's data) */
  TYPE_KIND_MODULE,
  TYPE_KIND_FUNCTION,
  TYPE_KIND_CALLBACK,
  TYPE_KIND_ERROR,
  /* Primitive */
  TYPE_KIND_VOID,
  TYPE_KIND_BOOL,
  TYPE_KIND_I8, TYPE_KIND_I16, TYPE_KIND_I32, TYPE_KIND_I64,
  TYPE_KIND_U8, TYPE_KIND_U16, TYPE_KIND_U32, TYPE_KIND_U64,
  TYPE_KIND_F16, TYPE_KIND_F32, TYPE_KIND_F64,
  TYPE_KIND_CHAR, TYPE_KIND_STR,
  TYPE_KIND_NIL,
  /* Composite */
  TYPE_KIND_POINTER, TYPE_KIND_ARRAY, TYPE_KIND_SLICE, TYPE_KIND_TUPLE,
  TYPE_KIND_STRUCT, TYPE_KIND_UNION, TYPE_KIND_CUNION,
  TYPE_KIND_ENUM, TYPE_KIND_INTERFACE,
} type_kind_t;

/**
 * @brief VTable — type behavior dispatch table.
 *        Embedded in stype_t, not a pointer.
 */
typedef struct vtable_t {
  value_t *(*clone)   (allocator_t alloc, value_t *obj);
  void     (*dispose) (allocator_t alloc, value_t *obj);
} vtable_t;

/**
 * @brief stype_t — type object (independent data structure).
 *
 * For TYPE_KIND_TYPE values: value.type and value.data point to the same
 * stype_t (type=ref, data=own).
 */
typedef struct stype_t {
  type_kind_t kind;
  const char *name;
  uint64_t    size;      /**< Byte size of the type's data */
  uint64_t    align;     /**< Alignment requirement */
  vtable_t    vtable;    /**< Behavior dispatch (embedded) */
} stype_t;

/* ---- Accessors ---- */

type_kind_t stype_get_kind(const stype_t *self);
const char *stype_get_name(const stype_t *self);
uint64_t    stype_get_size(const stype_t *self);
uint64_t    stype_get_align(const stype_t *self);

#ifdef __cplusplus
}
#endif
#endif
