#ifndef _H_CUBEC_C_IR_INCLUDE_
#define _H_CUBEC_C_IR_INCLUDE_
#include "c/c_ir.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief #include directive.
 */
typedef struct _c_ir_include_t *c_ir_include_t;

struct _c_ir_include_t {
  enum c_ir_kind kind;
  location_t source_loc;
  string_t path;      /**< Header path (e.g., "foo.h" or <stdint.h>) */
  bool is_system;     /**< true → <path>, false → "path" */
};

c_ir_include_t c_ir_include_create(allocator_t allocator, const char *path,
                                     bool is_system, location_t source_loc);
void c_ir_include_dispose(allocator_t allocator, c_ir_include_t *node);

#ifdef __cplusplus
}
#endif
#endif
