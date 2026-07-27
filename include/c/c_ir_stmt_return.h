#ifndef _H_CUBEC_C_IR_STMT_RETURN_
#define _H_CUBEC_C_IR_STMT_RETURN_
#include "c/c_ir.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Return statement: return expr; or return;
 */
typedef struct _c_ir_stmt_return_t *c_ir_stmt_return_t;

struct _c_ir_stmt_return_t {
  enum c_ir_kind kind;
  location_t source_loc;
  c_ir_node_t value;     /**< Return value expression, or NULL for bare return */
};

c_ir_stmt_return_t c_ir_stmt_return_create(allocator_t allocator, c_ir_node_t value,
                                             location_t source_loc);
void c_ir_stmt_return_dispose(allocator_t allocator, c_ir_stmt_return_t *node);

#ifdef __cplusplus
}
#endif
#endif
