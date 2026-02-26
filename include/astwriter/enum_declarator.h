#ifndef _H_CUBEC_ASTWRITER_ENUM_DECLARATOR_
#define _H_CUBEC_ASTWRITER_ENUM_DECLARATOR_
#include "ast/enum_declarator.h"
#include "core/any.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_any_t cubec_write_ast_enum_declarator(cubec_allocator_t allocator,
                                            cubec_ast_enum_declarator_t self);
#ifdef __cplusplus
}
#endif
#endif