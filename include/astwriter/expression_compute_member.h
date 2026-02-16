#ifndef _H_CUBEC_ASTWRITER_EXPRESSION_COMPUTE_MEMBER_
#define _H_CUBEC_ASTWRITER_EXPRESSION_COMPUTE_MEMBER_
#include "ast/expression_compute_member.h"
#include "core/value.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t cubec_write_ast_expression_compute_member(
    cubec_allocator_t allocator, cubec_ast_expression_compute_member_t self);
#ifdef __cplusplus
}
#endif
#endif