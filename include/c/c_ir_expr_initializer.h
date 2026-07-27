#ifndef _H_CUBEC_C_IR_EXPR_INITIALIZER_
#define _H_CUBEC_C_IR_EXPR_INITIALIZER_
#include "c/c_ir.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Designated initializer field: .name = value
 */
typedef struct _c_ir_expr_initializer_t *c_ir_expr_initializer_t;

struct _c_ir_expr_initializer_t {
  enum c_ir_kind kind;
  location_t source_loc;
  string_t name;        /**< Field name (or NULL for positional) */
  size_t index;         /**< Positional index (used when name is NULL) */
  c_ir_node_t value;
  bool is_designated;   /**< true → .name = value, false → positional */
};

c_ir_expr_initializer_t c_ir_expr_initializer_create(allocator_t allocator,
                                                        const char *name,
                                                        c_ir_node_t value,
                                                        location_t source_loc);
c_ir_expr_initializer_t c_ir_expr_initializer_create_indexed(allocator_t allocator,
                                                                size_t index,
                                                                c_ir_node_t value,
                                                                location_t source_loc);
void c_ir_expr_initializer_dispose(allocator_t allocator, c_ir_expr_initializer_t *node);

#ifdef __cplusplus
}
#endif
#endif
