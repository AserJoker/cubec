#ifndef _H_CUBEC_C_IR_UNIT_
#define _H_CUBEC_C_IR_UNIT_
#include "c/c_ir.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Compilation unit — one .c/.h file pair.
 */
typedef struct _c_ir_unit_t *c_ir_unit_t;

struct _c_ir_unit_t {
  enum c_ir_kind kind;
  location_t source_loc;
  string_t filename;       /**< Module name without extension */
  string_t module_hash;    /**< Short hash prefix (e.g., "m3a7") */
  bool is_library;         /**< true = generate library (export visibility) */
  vec_t includes;          /**< c_ir_node_t (C_IR_INCLUDE) */
  vec_t forward_decls;     /**< c_ir_node_t (C_IR_FORWARD_DECL) — forward decls only */
  vec_t struct_defs;       /**< c_ir_node_t (C_IR_FORWARD_DECL with body) — struct body definitions */
  vec_t typedefs;          /**< c_ir_node_t (C_IR_TYPEDEF) */
  vec_t enum_defs;         /**< c_ir_node_t (C_IR_ENUM_DEF) */
  vec_t variable_decls;    /**< c_ir_node_t (C_IR_VARIABLE_DECL) */
  vec_t function_decls;    /**< c_ir_node_t (C_IR_FUNCTION_DECL) — .h prototypes */
  vec_t function_defs;     /**< c_ir_node_t (C_IR_FUNCTION_DEF) — .c implementations */
  vec_t extern_decls;      /**< c_ir_node_t (C_IR_FUNCTION_DECL) — .c extern declarations */
};

/** @brief Create a compilation unit. */
c_ir_unit_t c_ir_unit_create(allocator_t allocator, const char *filename,
                               const char *module_hash, location_t source_loc);

/** @brief Dispose a compilation unit. */
void c_ir_unit_dispose(allocator_t allocator, c_ir_unit_t *unit);

#ifdef __cplusplus
}
#endif
#endif
