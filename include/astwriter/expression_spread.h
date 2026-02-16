#ifndef _H_CUBEC_ASTWRITER_EXPRESSION_SPREAD_
#define _H_CUBEC_ASTWRITER_EXPRESSION_SPREAD_
#include "ast/expression_spread.h"
#include "core/value.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t
cubec_write_ast_expression_spread(cubec_allocator_t allocator,
                                  cubec_ast_expression_spread_t self);
#ifdef __cplusplus
}
#endif
#endif