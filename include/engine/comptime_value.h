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
  COMPTIME_VALUE_STRUCT,    /**< struct instance — named fields */
  COMPTIME_VALUE_UNION,     /**< union instance — tagged variant */
  COMPTIME_VALUE_CUNION,    /**< C-union instance — C ABI tagged variant */
  COMPTIME_VALUE_TUPLE,     /**< tuple instance — ordered anonymous fields */
  COMPTIME_VALUE_ARRAY,     /**< array instance — indexed elements */
  COMPTIME_VALUE_SLICE,     /**< slice instance — ptr + start + length */
  COMPTIME_VALUE_CALLABLE,  /**< callable — function type reference */
  COMPTIME_VALUE_FUNCTION,  /**< function — concrete function binding */
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

/** Struct value. fields are owned (vec of comptime_value_t). */
struct _comptime_struct_t {
  struct _comptime_value_header_t header;
  vec_t fields;  /* owned: comptime_value_t array, ordered by struct layout */
};
typedef struct _comptime_struct_t *comptime_struct_t;

/** Union value. tag is the active variant index, value is the variant payload. */
struct _comptime_union_t {
  struct _comptime_value_header_t header;
  uint64_t tag;              /* active variant index */
  comptime_value_t value;    /* owned: variant payload */
};
typedef struct _comptime_union_t *comptime_union_t;

/** C-union value. No tag — like C union, programmer tracks active field. */
struct _comptime_cunion_t {
  struct _comptime_value_header_t header;
  comptime_value_t value;    /* owned: current field value */
};
typedef struct _comptime_cunion_t *comptime_cunion_t;

/** Tuple value. elements are owned (vec of comptime_value_t). */
struct _comptime_tuple_t {
  struct _comptime_value_header_t header;
  vec_t elements;  /* owned: comptime_value_t array */
};
typedef struct _comptime_tuple_t *comptime_tuple_t;

/** Array value. elements are owned (vec of comptime_value_t). */
struct _comptime_array_t {
  struct _comptime_value_header_t header;
  vec_t elements;  /* owned: comptime_value_t array */
};
typedef struct _comptime_array_t *comptime_array_t;

/** Callable value. function reference. */
struct _comptime_callable_t {
  struct _comptime_value_header_t header;
  void *function_ref;     /* borrowing: points to function_t or similar */
  uint64_t instance_hash; /* generic instance hash, 0 for non-generic */
};
typedef struct _comptime_callable_t *comptime_callable_t;

/** Slice value. ptr + start + length at comptime. */
struct _comptime_slice_t {
  struct _comptime_value_header_t header;
  comptime_value_t ptr;   /* borrowing: pointer to data */
  uint64_t start;         /* byte offset */
  uint64_t length;        /* element count */
};
typedef struct _comptime_slice_t *comptime_slice_t;

/** Function value. concrete function binding with captures. */
struct _comptime_function_t {
  struct _comptime_value_header_t header;
  void *function_ref;     /* borrowing: points to function_t */
  uint64_t instance_hash; /* generic instance hash */
  vec_t captures;         /* owned: captured comptime_value_t values (nullable) */
};
typedef struct _comptime_function_t *comptime_function_t;

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

/** @brief Compute structural hash of a comptime value. */
uint64_t comptime_value_hash(comptime_value_t val);

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
static inline comptime_struct_t comptime_value_as_struct(comptime_value_t val) {
  return (comptime_struct_t)val;
}
static inline comptime_union_t comptime_value_as_union(comptime_value_t val) {
  return (comptime_union_t)val;
}
static inline comptime_cunion_t comptime_value_as_cunion(comptime_value_t val) {
  return (comptime_cunion_t)val;
}
static inline comptime_tuple_t comptime_value_as_tuple(comptime_value_t val) {
  return (comptime_tuple_t)val;
}
static inline comptime_array_t comptime_value_as_array(comptime_value_t val) {
  return (comptime_array_t)val;
}
static inline comptime_slice_t comptime_value_as_slice(comptime_value_t val) {
  return (comptime_slice_t)val;
}
static inline comptime_callable_t comptime_value_as_callable(comptime_value_t val) {
  return (comptime_callable_t)val;
}
static inline comptime_function_t comptime_value_as_function(comptime_value_t val) {
  return (comptime_function_t)val;
}

#ifdef __cplusplus
}
#endif
#endif /* _H_CUBEC_ENGINE_COMPTIME_VALUE_ */
