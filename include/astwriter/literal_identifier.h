#ifndef _H_CUBEC_ASTWRITER_LITERAL_IDENTIFIER_
#define _H_CUBEC_ASTWRITER_LITERAL_IDENTIFIER_
#include "ast/literal_identifier.h"
#include "core/any.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_any_t cubec_write_ast_literal_identifier(
    cubec_allocator_t allocator,
    cubec_ast_literal_identifier_t literal_identifier);
#ifdef __cplusplus
}
#endif
#endif