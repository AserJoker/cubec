#ifndef _H_CUBEC_C_TYPE_
#define _H_CUBEC_C_TYPE_
#include "core/allocator.h"
#include "core/string.h"
#include "core/vec.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief C type representation using left/right split notation.
 *
 * C declaration syntax places modifiers on both sides of the name:
 *   int32_t *name[10]    → left="int32_t", right="*[10]"
 *   void (*name)(int32_t) → left="void", right="(*)(int32_t)"
 *   const char *name      → left="const char", right="*"
 *
 * Full type = left + " " + name + right
 */
typedef struct _c_type_t {
  string_t left;   /**< Type left part (e.g., "int32_t", "const m3a7_Point") */
  string_t right;  /**< Type right part (e.g., "*", "[10]", "(*)(int32_t)") */
} *c_type_t;

/** @brief Create a C type from left/right parts. */
c_type_t c_type_create(allocator_t allocator, const char *left,
                        const char *right);

/** @brief Create a simple/primitive type: left=name, right="". */
c_type_t c_type_primitive(allocator_t allocator, const char *name);

/** @brief Add pointer to right part: right += "*". Returns base. */
c_type_t c_type_pointer(allocator_t allocator, c_type_t base);

/** @brief Prepend "const " to left part. Returns base. */
c_type_t c_type_const(allocator_t allocator, c_type_t base);

/** @brief Prepend "volatile " to left part. Returns base. */
c_type_t c_type_volatile(allocator_t allocator, c_type_t base);

/** @brief Create a function pointer type. */
c_type_t c_type_function_ptr(allocator_t allocator, c_type_t return_type,
                               vec_t param_types, bool variadic);

/** @brief Add array dimension to right part: right += "[N]". Returns base. */
c_type_t c_type_array(allocator_t allocator, c_type_t base, size_t length);

/** @brief Free a C type. Sets *type to NULL. */
void c_type_dispose(allocator_t allocator, c_type_t *type);

#ifdef __cplusplus
}
#endif
#endif
