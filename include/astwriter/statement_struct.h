#ifndef _H_CUBEC_ASTWRITER_STATEMENT_STRUCT_
#define _H_CUBEC_ASTWRITER_STATEMENT_STRUCT_
#include "ast/statement_struct.h"
#include "core/value.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t
cubec_write_ast_statement_struct(cubec_allocator_t allocator,
                                 cubec_ast_statement_struct_t statement);
#ifdef __cplusplus
}
#endif
#endif