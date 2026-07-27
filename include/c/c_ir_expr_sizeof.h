#ifndef _H_CUBEC_C_IR_EXPR_SIZEOF_
#define _H_CUBEC_C_IR_EXPR_SIZEOF_
#include "c/c_ir.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _c_ir_expr_sizeof_t *c_ir_expr_sizeof_t;

struct _c_ir_expr_sizeof_t {
  enum c_ir_kind kind;
  location_t source_loc;
  c_type_t type;
};

c_ir_expr_sizeof_t c_ir_expr_sizeof_create(allocator_t allocator, c_type_t type,
                                              location_t source_loc);
void c_ir_expr_sizeof_dispose(allocator_t allocator, c_ir_expr_sizeof_t *node);

#ifdef __cplusplus
}
#endif
#endif
