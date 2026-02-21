#ifndef _H_CUBEC_ASTWRITER_STATEMENT_DO_WHILE_
#define _H_CUBEC_ASTWRITER_STATEMENT_DO_WHILE_
#include "ast/statement_do_while.h"
#include "core/value.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t
cubec_write_ast_statement_do_while(cubec_allocator_t allocator,
                                   cubec_ast_statement_do_while_t statement);
#ifdef __cplusplus
}
#endif
#endif