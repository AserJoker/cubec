#ifndef _H_CUBEC_C_IR_STMT_STMT_EXPR_
#define _H_CUBEC_C_IR_STMT_STMT_EXPR_
#include "c/c_ir.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief GCC statement expression: ({ stmts... }) — value of last stmt is the expression value.
 */
typedef struct _c_ir_stmt_stmt_expr_t *c_ir_stmt_stmt_expr_t;

struct _c_ir_stmt_stmt_expr_t {
  enum c_ir_kind kind;
  location_t source_loc;
  vec_t statements;    /**< c_ir_node_t — last one provides the value */
};

c_ir_stmt_stmt_expr_t c_ir_stmt_stmt_expr_create(allocator_t allocator, vec_t statements,
                                                    location_t source_loc);
void c_ir_stmt_stmt_expr_dispose(allocator_t allocator, c_ir_stmt_stmt_expr_t *node);

#ifdef __cplusplus
}
#endif
#endif
