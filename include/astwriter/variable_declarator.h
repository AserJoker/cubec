#ifndef _H_CUBEC_ASTWRITER_VARIABLE_DECLARATOR_
#define _H_CUBEC_ASTWRITER_VARIABLE_DECLARATOR_
#include "ast/variable_declarator.h"
#include "core/value.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t
cubec_write_ast_variable_declarator(cubec_allocator_t allocator,
                                    cubec_ast_variable_declarator_t self);
#ifdef __cplusplus
}
#endif
#endif