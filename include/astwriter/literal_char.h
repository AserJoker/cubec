#ifndef _H_CUBEC_ASTWRITER_LITERAL_CHAR_
#define _H_CUBEC_ASTWRITER_LITERAL_CHAR_
#include "ast/literal_char.h"
#include "core/any.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_any_t cubec_write_ast_literal_char(cubec_allocator_t allocator,
                                         cubec_ast_literal_char_t literal_char);
#ifdef __cplusplus
}
#endif
#endif