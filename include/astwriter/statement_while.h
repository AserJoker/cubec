#ifndef _H_CUBEC_ASTWRITER_STATEMENT_WHILE_
#define _H_CUBEC_ASTWRITER_STATEMENT_WHILE_
#include "ast/statement_while.h"
#include "core/any.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_any_t
cubec_write_ast_statement_while(cubec_allocator_t allocator,
                                cubec_ast_statement_while_t statement);
#ifdef __cplusplus
}
#endif
#endif