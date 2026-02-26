#ifndef _H_CUBEC_ASTWRITER_STATEMENT_CONTINUE_
#define _H_CUBEC_ASTWRITER_STATEMENT_CONTINUE_
#include "ast/statement_continue.h"
#include "core/any.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_any_t
cubec_write_ast_statement_continue(cubec_allocator_t allocator,
                                   cubec_ast_statement_continue_t self);
#ifdef __cplusplus
}
#endif
#endif