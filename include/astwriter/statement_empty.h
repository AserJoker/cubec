#ifndef _H_CUBEC_ASTWRITER_STATEMENT_EMPTY_
#define _H_CUBEC_ASTWRITER_STATEMENT_EMPTY_
#include "ast/statement_empty.h"
#include "core/any.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_any_t cubec_write_ast_statement_empty(cubec_allocator_t allocator,
                                            cubec_ast_statement_empty_t self);
#ifdef __cplusplus
}
#endif
#endif