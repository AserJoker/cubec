#ifndef _H_CUBEC_ENGINE_COMPTIME_VALUE_
#define _H_CUBEC_ENGINE_COMPTIME_VALUE_
#include "core/allocator.h"
#include "core/node.h"
#include "core/string.h"
#include "core/type.h"
#include "core/vec.h"
#include "engine/semantic_type.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Compile-time value kinds for the comptime interpreter.
 *
 * These represent the runtime values that the comptime evaluator
 * computes when walking the AST. Each value carries its semantic type
 * alongside the concrete data.
 */
enum comptime_value_kind {
  COMPTIME_VALUE_NIL,       /**< nil literal */
  COMPTIME_VALUE_BOOL,      /**< true / false */
  COMPTIME_VALUE_INT,       /**< i8..i64, u8..u64 */
  COMPTIME_VALUE_FLOAT,     /**< f16, f32, f64 */
  COMPTIME_VALUE_CHAR,      /**< char literal */
  COMPTIME_VALUE_STRING,    /**< string literal */
  COMPTIME_VALUE_TYPE,      /**< typeof result (semantic_type_t) */
  COMPTIME_VALUE_POINTER,   /**< virtual address */
  COMPTIME_VALUE_COMPOSITE, /**< struct/union instance */
  COMPTIME_VALUE_FUNCTION,  /**< closure (env + AST body) */
  COMPTIME_VALUE_ERROR,     /**< error sentinel */
};

/** @brief Forward declaration for comptime environment. */
struct comptime_env;
typedef struct comptime_env *comptime_env_t;

/**
 * @brief A discriminated union representing a compile-time value.
 */
struct comptime_value {
  enum comptime_value_kind kind;
  semantic_type_t type;     /**< Semantic type of this value */
  union {
    bool bool_val;
    struct { int64_t s; uint64_t u; uint8_t width; bool is_signed; } int_val;
    struct { double value; uint8_t width; } float_val;
    char char_val;
    string_t string_val;
    semantic_type_t type_val;
    struct { uint64_t addr; } pointer;
    struct { uint8_t *data; size_t data_size; semantic_type_t element_type; } composite;
    struct { comptime_env_t captured_env; node_t body; vec_t param_names; } function;
  };
};

typedef struct comptime_value *comptime_value_t;

/** @brief Virtual table for comptime_value_t. */
extern type_t g_comptime_value_type;

/* ===== constructors ===== */

comptime_value_t comptime_value_create_nil(allocator_t allocator,
                                           semantic_type_t type);
comptime_value_t comptime_value_create_bool(allocator_t allocator, bool val,
                                           semantic_type_t type);
comptime_value_t comptime_value_create_int(allocator_t allocator,
                                           int64_t sval, uint64_t uval,
                                           uint8_t width, bool is_signed,
                                           semantic_type_t type);
comptime_value_t comptime_value_create_float(allocator_t allocator,
                                             double val, uint8_t width,
                                             semantic_type_t type);
comptime_value_t comptime_value_create_char(allocator_t allocator, char val,
                                            semantic_type_t type);
comptime_value_t comptime_value_create_string(allocator_t allocator,
                                              const char *val,
                                              semantic_type_t type);
comptime_value_t comptime_value_create_type(allocator_t allocator,
                                            semantic_type_t val);
comptime_value_t comptime_value_create_pointer(allocator_t allocator,
                                               uint64_t addr,
                                               semantic_type_t type);
comptime_value_t comptime_value_create_composite(allocator_t allocator,
                                                  semantic_type_t type,
                                                  semantic_type_t element_type,
                                                  size_t data_size);
comptime_value_t comptime_value_create_function(allocator_t allocator,
                                                 comptime_env_t env,
                                                 node_t body,
                                                 vec_t param_names,
                                                 semantic_type_t type);
comptime_value_t comptime_value_create_error(allocator_t allocator);

/* ===== queries ===== */

bool comptime_value_is_truthy(comptime_value_t val);
bool comptime_value_equals(comptime_value_t a, comptime_value_t b);
comptime_value_t comptime_value_clone(allocator_t allocator,
                                      comptime_value_t src);

/** @brief Get the C string from a STRING value. Returns NULL for non-string kinds. */
const char *comptime_value_get_string(comptime_value_t val);

/* ===== mutation (field/index access for composite) ===== */

/** @brief Read a field value from composite's raw data at given offset. Returns owned value. */
comptime_value_t comptime_value_read_field(comptime_value_t composite,
                                           size_t offset,
                                           semantic_type_t field_type,
                                           allocator_t allocator);

/** @brief Write a field value into composite's raw data at given offset.
 *  field_type is used for implicit conversion and to determine write size. */
bool comptime_value_write_field(comptime_value_t composite,
                                size_t offset,
                                semantic_type_t field_type,
                                comptime_value_t value);

/** @brief Get a field value by name (struct). Returns owned value. */
comptime_value_t comptime_value_get_field(comptime_value_t composite,
                                          const char *field_name,
                                          allocator_t allocator);

/** @brief Set a field value by name (struct). */
bool comptime_value_set_field(comptime_value_t composite,
                              const char *field_name,
                              comptime_value_t value);

/** @brief Get an array element by index. Returns owned value. */
comptime_value_t comptime_value_get_index(comptime_value_t composite,
                                          size_t index,
                                          allocator_t allocator);

/** @brief Set an array element by index. */
bool comptime_value_set_index(comptime_value_t composite,
                              size_t index,
                              comptime_value_t value);

/* ===== conversions ===== */

int64_t  comptime_value_as_i64(comptime_value_t val);
uint64_t comptime_value_as_u64(comptime_value_t val);
double   comptime_value_as_f64(comptime_value_t val);

#ifdef __cplusplus
}
#endif
#endif
