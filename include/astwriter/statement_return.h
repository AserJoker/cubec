#ifndef _H_CUBEC_ASTWRITER_STATEMENT_RETURN_
#define _H_CUBEC_ASTWRITER_STATEMENT_RETURN_
#include "ast/statement_return.h"
#include "core/any.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_any_t cubec_write_ast_statement_return(cubec_allocator_t allocator,
                                             cubec_ast_statement_return_t self);
#ifdef __cplusplus
}
#endif
#endif