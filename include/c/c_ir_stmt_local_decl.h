#ifndef _H_CUBEC_C_IR_STMT_LOCAL_DECL_
#define _H_CUBEC_C_IR_STMT_LOCAL_DECL_
#include "c/c_ir.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Local variable declaration: type name = init;
 */
typedef struct _c_ir_stmt_local_decl_t *c_ir_stmt_local_decl_t;

struct _c_ir_stmt_local_decl_t {
  enum c_ir_kind kind;
  location_t source_loc;
  c_type_t type;
  string_t name;
  c_ir_node_t init;        /**< Initializer expression, or NULL */
};

c_ir_stmt_local_decl_t c_ir_stmt_local_decl_create(allocator_t allocator,
                                                      c_type_t type,
                                                      const char *name,
                                                      c_ir_node_t init,
                                                      location_t source_loc);
void c_ir_stmt_local_decl_dispose(allocator_t allocator, c_ir_stmt_local_decl_t *node);

#ifdef __cplusplus
}
#endif
#endif
