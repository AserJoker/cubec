#ifndef _H_CUBEC_ASTWRITER_STATEMENT_DEFER_
#define _H_CUBEC_ASTWRITER_STATEMENT_DEFER_
#include "ast/statement_defer.h"
#include "core/any.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_any_t cubec_write_ast_statement_defer(cubec_allocator_t allocator,
                                            cubec_ast_statement_defer_t self);
#ifdef __cplusplus
}
#endif
#endif