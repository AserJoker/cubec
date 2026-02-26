#ifndef _H_CUBEC_ASTWRITER_STATEMENT_FOR_
#define _H_CUBEC_ASTWRITER_STATEMENT_FOR_
#include "ast/statement_for.h"
#include "core/any.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_any_t cubec_write_ast_statement_for(cubec_allocator_t allocator,
                                          cubec_ast_statement_for_t statement);
#ifdef __cplusplus
}
#endif
#endif