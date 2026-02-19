#ifndef _H_CUBEC_ASTWRITER_STATEMENT_FUNCTION_
#define _H_CUBEC_ASTWRITER_STATEMENT_FUNCTION_
#include "ast/statement_function.h"
#include "core/value.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t
cubec_write_ast_statement_function(cubec_allocator_t allocator,
                                   cubec_ast_statement_function_t statement);
#ifdef __cplusplus
}
#endif
#endif