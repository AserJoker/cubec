#ifndef _H_CUBEC_C_IR_EXPR_TERNARY_
#define _H_CUBEC_C_IR_EXPR_TERNARY_
#include "c/c_ir.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _c_ir_expr_ternary_t *c_ir_expr_ternary_t;

struct _c_ir_expr_ternary_t {
  enum c_ir_kind kind;
  location_t source_loc;
  c_ir_node_t condition;
  c_ir_node_t consequent;
  c_ir_node_t alternate;
};

c_ir_expr_ternary_t c_ir_expr_ternary_create(allocator_t allocator,
                                                c_ir_node_t condition,
                                                c_ir_node_t consequent,
                                                c_ir_node_t alternate,
                                                location_t source_loc);
void c_ir_expr_ternary_dispose(allocator_t allocator, c_ir_expr_ternary_t *node);

#ifdef __cplusplus
}
#endif
#endif
