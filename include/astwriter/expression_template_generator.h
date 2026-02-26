#ifndef _H_CUBEC_ASTWRITER_EXPRESSION_TEMPLATE_GENERATOR_
#define _H_CUBEC_ASTWRITER_EXPRESSION_TEMPLATE_GENERATOR_
#include "ast/expression_template_generator.h"
#include "core/any.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_any_t cubec_write_ast_expression_template_generator(
    cubec_allocator_t allocator,
    cubec_ast_expression_template_generator_t self);
#ifdef __cplusplus
}
#endif
#endif