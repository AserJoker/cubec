#ifndef _H_CUBEC_C_IR_FUNCTION_
#define _H_CUBEC_C_IR_FUNCTION_
#include "c/c_ir.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Function parameter.
 */
typedef struct _c_ir_param_t {
  c_type_t type;
  string_t name;
} *c_ir_param_t;

/** @brief Create a function parameter. */
c_ir_param_t c_ir_param_create(allocator_t allocator, c_type_t type,
                                 const char *name);

/** @brief Dispose a function parameter. */
void c_ir_param_dispose(allocator_t allocator, c_ir_param_t *param);

/**
 * @brief Function definition (with body).
 */
typedef struct _c_ir_function_def_t *c_ir_function_def_t;

struct _c_ir_function_def_t {
  enum c_ir_kind kind;
  location_t source_loc;
  c_type_t return_type;
  string_t name;           /**< Mangled name */
  vec_t params;            /**< c_ir_param_t */
  bool is_static;
  bool is_inline;
  bool is_hidden;          /**< __attribute__((no_instrument_function, noinline)) */
  bool is_artificial;      /**< __attribute__((artificial)) */
  bool is_c_variadic;      /**< C-style variadic (...) */
  c_ir_node_t body;        /**< C_IR_STMT_BLOCK */
};

c_ir_function_def_t c_ir_function_def_create(allocator_t allocator,
                                               c_type_t return_type,
                                               const char *name,
                                               vec_t params,
                                               bool is_static, bool is_inline,
                                               bool is_hidden, bool is_artificial,
                                               bool is_c_variadic,
                                               c_ir_node_t body,
                                               location_t source_loc);
void c_ir_function_def_dispose(allocator_t allocator, c_ir_function_def_t *node);

/**
 * @brief Function declaration (prototype).
 */
typedef struct _c_ir_function_decl_t *c_ir_function_decl_t;

struct _c_ir_function_decl_t {
  enum c_ir_kind kind;
  location_t source_loc;
  c_type_t return_type;
  string_t name;
  vec_t params;            /**< c_ir_param_t */
  bool is_static;
  bool is_inline;
  bool is_hidden;
  bool is_artificial;
  bool is_c_variadic;      /**< C-style variadic (...) */
};

c_ir_function_decl_t c_ir_function_decl_create(allocator_t allocator,
                                                 c_type_t return_type,
                                                 const char *name,
                                                 vec_t params,
                                                 bool is_static, bool is_inline,
                                                 bool is_hidden, bool is_artificial,
                                                 bool is_c_variadic,
                                                 location_t source_loc);
void c_ir_function_decl_dispose(allocator_t allocator, c_ir_function_decl_t *node);

#ifdef __cplusplus
}
#endif
#endif
