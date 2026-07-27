#ifndef _H_CUBEC_C_IR_EXPR_BINARY_
#define _H_CUBEC_C_IR_EXPR_BINARY_
#include "c/c_ir.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _c_ir_expr_binary_t *c_ir_expr_binary_t;

struct _c_ir_expr_binary_t {
  enum c_ir_kind kind;
  location_t source_loc;
  string_t op;           /**< Operator: "+", "-", "==", "&&", etc. */
  c_ir_node_t left;
  c_ir_node_t right;
};

c_ir_expr_binary_t c_ir_expr_binary_create(allocator_t allocator, const char *op,
                                             c_ir_node_t left, c_ir_node_t right,
                                             location_t source_loc);
void c_ir_expr_binary_dispose(allocator_t allocator, c_ir_expr_binary_t *node);

#ifdef __cplusplus
}
#endif
#endif
