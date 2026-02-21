#ifndef _H_CUBEC_ASTWRITER_STATEMENT_IF_
#define _H_CUBEC_ASTWRITER_STATEMENT_IF_
#include "ast/statement_if.h"
#include "core/value.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t cubec_write_ast_statement_if(cubec_allocator_t allocator,
                                           cubec_ast_statement_if_t statement);
#ifdef __cplusplus
}
#endif
#endif