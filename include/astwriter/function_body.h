#ifndef _H_CUBEC_ASTWRITER_FUNCTION_BODY_
#define _H_CUBEC_ASTWRITER_FUNCTION_BODY_
#include "ast/function_body.h"
#include "core/any.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_any_t cubec_write_ast_function_body(cubec_allocator_t allocator,
                                          cubec_ast_function_body_t self);
#ifdef __cplusplus
}
#endif
#endif