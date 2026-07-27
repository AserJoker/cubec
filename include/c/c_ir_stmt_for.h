#ifndef _H_CUBEC_C_IR_STMT_FOR_
#define _H_CUBEC_C_IR_STMT_FOR_
#include "c/c_ir.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _c_ir_stmt_for_t *c_ir_stmt_for_t;

struct _c_ir_stmt_for_t {
  enum c_ir_kind kind;
  location_t source_loc;
  c_ir_node_t init;         /**< Init expression/statement, or NULL */
  c_ir_node_t condition;    /**< Loop condition, or NULL */
  c_ir_node_t update;       /**< Update expression, or NULL */
  c_ir_node_t body;
};

c_ir_stmt_for_t c_ir_stmt_for_create(allocator_t allocator,
                                       c_ir_node_t init, c_ir_node_t condition,
                                       c_ir_node_t update, c_ir_node_t body,
                                       location_t source_loc);
void c_ir_stmt_for_dispose(allocator_t allocator, c_ir_stmt_for_t *node);

#ifdef __cplusplus
}
#endif
#endif
