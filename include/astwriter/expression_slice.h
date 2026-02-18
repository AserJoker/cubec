#ifndef _H_CUBEC_ASTWRITER_EXPRESSION_SLICE_
#define _H_CUBEC_ASTWRITER_EXPRESSION_SLICE_
#include "ast/expression_slice.h"
#include "core/value.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t
cubec_write_ast_expression_slice(cubec_allocator_t allocator,
                                 cubec_ast_expression_slice_t self);
#ifdef __cplusplus
}
#endif
#endif