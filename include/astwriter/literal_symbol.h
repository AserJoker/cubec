#ifndef _H_CUBEC_ASTWRITER_LITERAL_SYMBOL_
#define _H_CUBEC_ASTWRITER_LITERAL_SYMBOL_
#include "ast/literal_symbol.h"
#include "core/value.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t cubec_write_ast_literal_symbol(
    cubec_allocator_t allocator,
    cubec_ast_literal_symbol_t literal_symbol);
#ifdef __cplusplus
}
#endif
#endif