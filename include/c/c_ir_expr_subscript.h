#ifndef _H_CUBEC_C_IR_EXPR_SUBSCRIPT_
#define _H_CUBEC_C_IR_EXPR_SUBSCRIPT_
#include "c/c_ir.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _c_ir_expr_subscript_t *c_ir_expr_subscript_t;

struct _c_ir_expr_subscript_t {
  enum c_ir_kind kind;
  location_t source_loc;
  c_ir_node_t object;
  c_ir_node_t index;
};

c_ir_expr_subscript_t c_ir_expr_subscript_create(allocator_t allocator,
                                                    c_ir_node_t object,
                                                    c_ir_node_t index,
                                                    location_t source_loc);
void c_ir_expr_subscript_dispose(allocator_t allocator, c_ir_expr_subscript_t *node);

#ifdef __cplusplus
}
#endif
#endif
