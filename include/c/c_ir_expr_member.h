#ifndef _H_CUBEC_C_IR_EXPR_MEMBER_
#define _H_CUBEC_C_IR_EXPR_MEMBER_
#include "c/c_ir.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _c_ir_expr_member_t *c_ir_expr_member_t;

struct _c_ir_expr_member_t {
  enum c_ir_kind kind;
  location_t source_loc;
  c_ir_node_t object;
  string_t field;
  bool is_arrow;          /**< true → ->, false → . */
};

c_ir_expr_member_t c_ir_expr_member_create(allocator_t allocator, c_ir_node_t object,
                                             const char *field, bool is_arrow,
                                             location_t source_loc);
void c_ir_expr_member_dispose(allocator_t allocator, c_ir_expr_member_t *node);

#ifdef __cplusplus
}
#endif
#endif
