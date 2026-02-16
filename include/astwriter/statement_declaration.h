#ifndef _H_CUBEC_ASTWRITER_STATEMENT_DECLARATION_
#define _H_CUBEC_ASTWRITER_STATEMENT_DECLARATION_
#include "ast/statement_declaration.h"
#include "core/value.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t
cubec_write_ast_statement_declaration(cubec_allocator_t allocator,
                                      cubec_ast_statement_declaration_t self);
#ifdef __cplusplus
}
#endif
#endif