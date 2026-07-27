#ifndef _H_CUBEC_C_IR_FORWARD_DECL_
#define _H_CUBEC_C_IR_FORWARD_DECL_
#include "c/c_ir.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Forward declaration: typedef struct Name Name;
 */
typedef struct _c_ir_forward_decl_t *c_ir_forward_decl_t;

struct _c_ir_forward_decl_t {
  enum c_ir_kind kind;
  location_t source_loc;
  string_t name;    /**< Tag name (e.g., "m3a7_Point") */
};

c_ir_forward_decl_t c_ir_forward_decl_create(allocator_t allocator,
                                               const char *name,
                                               location_t source_loc);
void c_ir_forward_decl_dispose(allocator_t allocator, c_ir_forward_decl_t *node);

#ifdef __cplusplus
}
#endif
#endif
