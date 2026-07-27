#ifndef _H_CUBEC_C_IR_STMT_EXPR_
#define _H_CUBEC_C_IR_STMT_EXPR_
#include "c/c_ir.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Expression statement: expr;
 */
typedef struct _c_ir_stmt_expr_t *c_ir_stmt_expr_t;

struct _c_ir_stmt_expr_t {
  enum c_ir_kind kind;
  location_t source_loc;
  c_ir_node_t expression;
};

c_ir_stmt_expr_t c_ir_stmt_expr_create(allocator_t allocator, c_ir_node_t expression,
                                          location_t source_loc);
void c_ir_stmt_expr_dispose(allocator_t allocator, c_ir_stmt_expr_t *node);

#ifdef __cplusplus
}
#endif
#endif
