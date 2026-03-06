#ifndef _H_CUBEC_ASTWRITER_TYPE_
#define _H_CUBEC_ASTWRITER_TYPE_
#include "ast/type.h"
#include "core/any.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_any_t cubec_write_ast_type(cubec_allocator_t allocator,
                                 cubec_ast_type_t self);
#ifdef __cplusplus
}
#endif
#endif