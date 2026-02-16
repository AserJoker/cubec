#ifndef _H_CUBEC_ASTWRITER_EXPRESSION_ASSIGMENT_
#define _H_CUBEC_ASTWRITER_EXPRESSION_ASSIGMENT_
#include "ast/expression_assigment.h"
#include "core/value.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t
cubec_write_ast_expression_assigment(cubec_allocator_t allocator,
                                     cubec_ast_expression_assigment_t self);
#ifdef __cplusplus
}
#endif
#endif