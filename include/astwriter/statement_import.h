#ifndef _H_CUBEC_ASTWRITER_STATEMENT_IMPORT_
#define _H_CUBEC_ASTWRITER_STATEMENT_IMPORT_
#include "ast/statement_import.h"
#include "core/any.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_any_t
cubec_write_ast_statement_import(cubec_allocator_t allocator,
                                 cubec_ast_statement_import_t statement);
#ifdef __cplusplus
}
#endif
#endif