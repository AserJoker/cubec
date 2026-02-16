#ifndef _H_CUBEC_ASTWRITER_LITERAL_NUMERIC_
#define _H_CUBEC_ASTWRITER_LITERAL_NUMERIC_
#include "ast/literal_numeric.h"
#include "core/value.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t cubec_write_ast_literal_numeric(cubec_allocator_t allocator,
                                              cubec_ast_literal_numeric_t self);
#ifdef __cplusplus
}
#endif
#endif