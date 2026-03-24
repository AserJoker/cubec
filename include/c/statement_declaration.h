#ifndef _H_CUBEC_C_STATEMENT_DECLARATION_
#define _H_CUBEC_C_STATEMENT_DECLARATION_
#include "ast/statement_declaration.h"
#include "core/string.h"
#include "engine/context.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t cubec_c_write_statement_declaration(
    cubec_context_t self, cubec_ast_statement_declaration_t sts,
    const char *filename, cubec_string_t *output);
#ifdef __cplusplus
}
#endif
#endif