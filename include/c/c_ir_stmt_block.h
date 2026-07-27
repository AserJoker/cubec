#ifndef _H_CUBEC_C_IR_STMT_BLOCK_
#define _H_CUBEC_C_IR_STMT_BLOCK_
#include "c/c_ir.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Block statement: { stmts... }
 */
typedef struct _c_ir_stmt_block_t *c_ir_stmt_block_t;

struct _c_ir_stmt_block_t {
  enum c_ir_kind kind;
  location_t source_loc;
  vec_t statements;    /**< c_ir_node_t */
};

c_ir_stmt_block_t c_ir_stmt_block_create(allocator_t allocator, vec_t statements,
                                           location_t source_loc);
void c_ir_stmt_block_dispose(allocator_t allocator, c_ir_stmt_block_t *node);

#ifdef __cplusplus
}
#endif
#endif
