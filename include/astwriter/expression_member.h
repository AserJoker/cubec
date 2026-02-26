#ifndef _H_CUBEC_ASTWRITER_EXPRESSION_MEMBER_
#define _H_CUBEC_ASTWRITER_EXPRESSION_MEMBER_
#include "ast/expression_member.h"
#include "core/any.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_any_t
cubec_write_ast_expression_member(cubec_allocator_t allocator,
                                  cubec_ast_expression_member_t self);
#ifdef __cplusplus
}
#endif
#endif