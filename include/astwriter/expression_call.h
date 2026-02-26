#ifndef _H_CUBEC_ASTWRITER_EXPRESSION_CALL_
#define _H_CUBEC_ASTWRITER_EXPRESSION_CALL_
#include "ast/expression_call.h"
#include "core/any.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_any_t cubec_write_ast_expression_call(cubec_allocator_t allocator,
                                            cubec_ast_expression_call_t self);
#ifdef __cplusplus
}
#endif
#endif