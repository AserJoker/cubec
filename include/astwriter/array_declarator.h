#ifndef _H_CUBEC_ASTWRITER_ARRAY_DECLARATOR_
#define _H_CUBEC_ASTWRITER_ARRAY_DECLARATOR_
#include "ast/array_declarator.h"
#include "core/any.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_any_t cubec_write_ast_array_declarator(cubec_allocator_t allocator,
                                             cubec_ast_array_declarator_t self);
#ifdef __cplusplus
}
#endif
#endif