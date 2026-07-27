#ifndef _H_CUBEC_C_IR_STMT_WHILE_
#define _H_CUBEC_C_IR_STMT_WHILE_
#include "c/c_ir.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _c_ir_stmt_while_t *c_ir_stmt_while_t;

struct _c_ir_stmt_while_t {
  enum c_ir_kind kind;
  location_t source_loc;
  c_ir_node_t condition;
  c_ir_node_t body;
};

c_ir_stmt_while_t c_ir_stmt_while_create(allocator_t allocator, c_ir_node_t condition,
                                           c_ir_node_t body, location_t source_loc);
void c_ir_stmt_while_dispose(allocator_t allocator, c_ir_stmt_while_t *node);

#ifdef __cplusplus
}
#endif
#endif
