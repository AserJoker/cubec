#ifndef _H_CUBEC_ASTWRITER_EXPRESSION_BINARY_
#define _H_CUBEC_ASTWRITER_EXPRESSION_BINARY_
#include "ast/expression_binary.h"
#include "core/value.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t
cubec_write_ast_expression_binary(cubec_allocator_t allocator,
                                  cubec_ast_expression_binary_t self);
#ifdef __cplusplus
}
#endif
#endif