#ifndef _H_CUBEC_ASTWRITER_STATEMENT_BREAK_
#define _H_CUBEC_ASTWRITER_STATEMENT_BREAK_
#include "ast/statement_break.h"
#include "core/value.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t cubec_write_ast_statement_break(cubec_allocator_t allocator,
                                              cubec_ast_statement_break_t self);
#ifdef __cplusplus
}
#endif
#endif