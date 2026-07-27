#ifndef _H_CUBEC_C_IR_TYPEDEF_
#define _H_CUBEC_C_IR_TYPEDEF_
#include "c/c_ir.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief typedef declaration.
 */
typedef struct _c_ir_typedef_t *c_ir_typedef_t;

struct _c_ir_typedef_t {
  enum c_ir_kind kind;
  location_t source_loc;
  c_type_t type;
  string_t name;
};

c_ir_typedef_t c_ir_typedef_create(allocator_t allocator, c_type_t type,
                                     const char *name, location_t source_loc);
void c_ir_typedef_dispose(allocator_t allocator, c_ir_typedef_t *node);

#ifdef __cplusplus
}
#endif
#endif
