#ifndef _H_CUBEC_ASTWRITER_STATEMENT_EXPRESSION_
#define _H_CUBEC_ASTWRITER_STATEMENT_EXPRESSION_
#include "ast/statement_expression.h"
#include "core/any.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_any_t cubec_write_ast_statement_expression(
    cubec_allocator_t allocator, cubec_ast_statement_expression_t statement);
#ifdef __cplusplus
}
#endif
#endif