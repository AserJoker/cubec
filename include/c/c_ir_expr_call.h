#ifndef _H_CUBEC_C_IR_EXPR_CALL_
#define _H_CUBEC_C_IR_EXPR_CALL_
#include "c/c_ir.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _c_ir_expr_call_t *c_ir_expr_call_t;

struct _c_ir_expr_call_t {
  enum c_ir_kind kind;
  location_t source_loc;
  c_ir_node_t callee;
  vec_t arguments;       /**< c_ir_node_t */
};

c_ir_expr_call_t c_ir_expr_call_create(allocator_t allocator, c_ir_node_t callee,
                                         vec_t arguments, location_t source_loc);
void c_ir_expr_call_dispose(allocator_t allocator, c_ir_expr_call_t *node);

#ifdef __cplusplus
}
#endif
#endif
