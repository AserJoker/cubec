#ifndef _H_CUBEC_C_IR_EXPR_UNARY_
#define _H_CUBEC_C_IR_EXPR_UNARY_
#include "c/c_ir.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _c_ir_expr_unary_t *c_ir_expr_unary_t;

struct _c_ir_expr_unary_t {
  enum c_ir_kind kind;
  location_t source_loc;
  string_t op;           /**< Operator: "!", "-", "*", "&", "++", "--" */
  c_ir_node_t operand;
  bool is_prefix;        /**< true for prefix (++x), false for postfix (x++) */
};

c_ir_expr_unary_t c_ir_expr_unary_create(allocator_t allocator, const char *op,
                                           c_ir_node_t operand, bool is_prefix,
                                           location_t source_loc);
void c_ir_expr_unary_dispose(allocator_t allocator, c_ir_expr_unary_t *node);

#ifdef __cplusplus
}
#endif
#endif
