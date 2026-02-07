#ifndef _H_CUBEC_ASTWRITER_STATEMENT_IMPORT_
#define _H_CUBEC_ASTWRITER_STATEMENT_IMPORT_
#include "ast/statement_import.h"
#include "core/value.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t
cubec_write_ast_statement_import(cubec_allocator_t allocator,
                                 cubec_ast_statement_import_t statement);
#ifdef __cplusplus
}
#endif
#endif