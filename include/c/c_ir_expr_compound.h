#ifndef _H_CUBEC_C_IR_EXPR_COMPOUND_
#define _H_CUBEC_C_IR_EXPR_COMPOUND_
#include "c/c_ir.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Compound literal: (type){ .field = value, ... }
 */
typedef struct _c_ir_expr_compound_t *c_ir_expr_compound_t;

struct _c_ir_expr_compound_t {
  enum c_ir_kind kind;
  location_t source_loc;
  c_type_t type;
  vec_t fields;          /**< c_ir_node_t (C_IR_EXPR_INITIALIZER entries) */
};

c_ir_expr_compound_t c_ir_expr_compound_create(allocator_t allocator, c_type_t type,
                                                 vec_t fields, location_t source_loc);
void c_ir_expr_compound_dispose(allocator_t allocator, c_ir_expr_compound_t *node);

#ifdef __cplusplus
}
#endif
#endif
