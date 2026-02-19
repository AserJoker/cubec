#ifndef _H_CUBEC_ASTWRITER_STATEMENT_ENUM_
#define _H_CUBEC_ASTWRITER_STATEMENT_ENUM_
#include "ast/statement_enum.h"
#include "core/value.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t
cubec_write_ast_statement_enum(cubec_allocator_t allocator,
                               cubec_ast_statement_enum_t statement);
#ifdef __cplusplus
}
#endif
#endif