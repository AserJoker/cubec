#ifndef _H_CUBEC_ASTWRITER_EXPRESSION_COMMA_
#define _H_CUBEC_ASTWRITER_EXPRESSION_COMMA_
#include "ast/expression_comma.h"
#include "core/any.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_any_t cubec_write_ast_expression_comma(cubec_allocator_t allocator,
                                             cubec_ast_expression_comma_t self);
#ifdef __cplusplus
}
#endif
#endif