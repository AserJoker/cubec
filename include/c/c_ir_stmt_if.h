#ifndef _H_CUBEC_C_IR_STMT_IF_
#define _H_CUBEC_C_IR_STMT_IF_
#include "c/c_ir.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _c_ir_stmt_if_t *c_ir_stmt_if_t;

struct _c_ir_stmt_if_t {
  enum c_ir_kind kind;
  location_t source_loc;
  c_ir_node_t condition;
  c_ir_node_t then_branch;
  c_ir_node_t else_branch;    /**< NULL if no else */
};

c_ir_stmt_if_t c_ir_stmt_if_create(allocator_t allocator, c_ir_node_t condition,
                                     c_ir_node_t then_branch, c_ir_node_t else_branch,
                                     location_t source_loc);
void c_ir_stmt_if_dispose(allocator_t allocator, c_ir_stmt_if_t *node);

#ifdef __cplusplus
}
#endif
#endif
