#ifndef _H_CUBEC_ASTWRITER_LITERAL_STRING_
#define _H_CUBEC_ASTWRITER_LITERAL_STRING_
#include "ast/literal_string.h"
#include "core/value.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t
cubec_write_ast_literal_string(cubec_allocator_t allocator,
                               cubec_ast_literal_string_t literal_string);
#ifdef __cplusplus
}
#endif
#endif