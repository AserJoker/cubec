#ifndef _H_CUBEC_ASTWRITER_FUNCTION_SELF_
#define _H_CUBEC_ASTWRITER_FUNCTION_SELF_
#include "ast/function_self.h"
#include "core/value.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t cubec_write_ast_function_self(cubec_allocator_t allocator,
                                            cubec_ast_function_self_t self);
#ifdef __cplusplus
}
#endif
#endif