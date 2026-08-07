#ifndef _H_CUBEC_ENGINE_COMPTIME_VALUE_
#define _H_CUBEC_ENGINE_COMPTIME_VALUE_

#include "core/allocator.h"
#include "core/string.h"
#include "engine/stype.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Compile-time value kind classification.
 */
enum comptime_value_kind {
  COMPTIME_VALUE_INT,       /**< integer: i8~i64, u8~u64, char */
  COMPTIME_VALUE_FLOAT,     /**< float: f16, f32, f64 */
  COMPTIME_VALUE_BOOL,      /**< bool */
  COMPTIME_VALUE_STRING,    /**< str — owned string */
  COMPTIME_VALUE_NIL,       /**< nil — typed null pointer */
  COMPTIME_VALUE_COMPOSITE, /**< struct/union/array/tuple instance */
};

/**
 * @brief Common header for all comptime values.
 *
 * Each comptime_value_kind has its own struct with this as the first field.
 * Guarantees layout compatibility for type-erased access.
 * type is borrowing — points to the stype_t that describes this value.
 */
struct _comptime_value_header_t {
  allocator_t allocator;
  enum comptime_value_kind kind;
  stype_t type;  /* borrowing: the semantic type of this value */
};
typedef struct _comptime_value_header_t *comptime_value_t;

/* --------------------------------------------------------------------------
 *  Per-kind structs
 * -------------------------------------------------------------------------- */

/** Integer value (i8~i64, u8~u64, char). Width/signedness via header.type. */
struct _comptime_int_t {
  struct _comptime_value_header_t header;
  uint64_t value;
};
typedef struct _comptime_int_t *comptime_int_t;

/** Float value (f16, f32, f64). Precision via header.type. */
struct _comptime_float_t {
  struct _comptime_value_header_t header;
  double value;
};
typedef struct _comptime_float_t *comptime_float_t;

/** Bool value. */
struct _comptime_bool_t {
  struct _comptime_value_header_t header;
  bool value;
};
typedef struct _comptime_bool_t *comptime_bool_t;

/** String value. value is owned. */
struct _comptime_string_t {
  struct _comptime_value_header_t header;
  string_t value;  /* owned */
};
typedef struct _comptime_string_t *comptime_string_t;

/** Nil value. No payload. */
struct _comptime_nil_t {
  struct _comptime_value_header_t header;
};
typedef struct _comptime_nil_t *comptime_nil_t;

/** Composite value (struct/union/array/tuple). fields are owned. */
struct _comptime_composite_t {
  struct _comptime_value_header_t header;
  vec_t fields;  /* comptime_value_t array, owned, auto_dispose */
};
typedef struct _comptime_composite_t *comptime_composite_t;

/* --------------------------------------------------------------------------
 *  Core API
 * -------------------------------------------------------------------------- */

/** @brief Get the kind of a comptime value. */
static inline enum comptime_value_kind comptime_value_get_kind(comptime_value_t val) {
  return val->kind;
}

/** @brief Get the stype of a comptime value (borrowing). */
static inline stype_t comptime_value_get_type(comptime_value_t val) {
  return val->type;
}

/** @brief Dispose a comptime value and its owned sub-objects. */
void comptime_value_dispose(comptime_value_t val);

/** @brief Clone a comptime value (deep copy). */
comptime_value_t comptime_value_clone(allocator_t allocator, comptime_value_t val);

/* --------------------------------------------------------------------------
 *  Type-safe downcast helpers
 * -------------------------------------------------------------------------- */

static inline comptime_int_t comptime_value_as_int(comptime_value_t val) {
  return (comptime_int_t)val;
}
static inline comptime_float_t comptime_value_as_float(comptime_value_t val) {
  return (comptime_float_t)val;
}
static inline comptime_bool_t comptime_value_as_bool(comptime_value_t val) {
  return (comptime_bool_t)val;
}
static inline comptime_string_t comptime_value_as_string(comptime_value_t val) {
  return (comptime_string_t)val;
}
static inline comptime_nil_t comptime_value_as_nil(comptime_value_t val) {
  return (comptime_nil_t)val;
}
static inline comptime_composite_t comptime_value_as_composite(comptime_value_t val) {
  return (comptime_composite_t)val;
}

#ifdef __cplusplus
}
#endif
#endif /* _H_CUBEC_ENGINE_COMPTIME_VALUE_ */
