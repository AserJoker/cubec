#ifndef _H_CUBEC_C_IR_VARIABLE_
#define _H_CUBEC_C_IR_VARIABLE_
#include "c/c_ir.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Global or local variable declaration.
 */
typedef struct _c_ir_variable_decl_t *c_ir_variable_decl_t;

struct _c_ir_variable_decl_t {
  enum c_ir_kind kind;
  location_t source_loc;
  c_type_t type;
  string_t name;
  c_ir_node_t init;        /**< Initializer expression, or NULL */
  bool is_static;
  bool is_extern;
};

c_ir_variable_decl_t c_ir_variable_decl_create(allocator_t allocator,
                                                 c_type_t type,
                                                 const char *name,
                                                 c_ir_node_t init,
                                                 bool is_static, bool is_extern,
                                                 location_t source_loc);
void c_ir_variable_decl_dispose(allocator_t allocator, c_ir_variable_decl_t *node);

#ifdef __cplusplus
}
#endif
#endif
