#ifndef _H_CUBEC_ASTWRITER_EXPRESSION_GROUP_
#define _H_CUBEC_ASTWRITER_EXPRESSION_GROUP_
#include "ast/expression_group.h"
#include "core/value.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t
cubec_write_ast_expression_group(cubec_allocator_t allocator,
                                 cubec_ast_expression_group_t self);
#ifdef __cplusplus
}
#endif
#endif