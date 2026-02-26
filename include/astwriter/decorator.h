#ifndef _H_CUBEC_ASTWRITER_DECORATOR_
#define _H_CUBEC_ASTWRITER_DECORATOR_
#include "ast/decorator.h"
#include "core/any.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_any_t cubec_write_ast_decorator(cubec_allocator_t allocator,
                                      cubec_ast_decorator_t self);
#ifdef __cplusplus
}
#endif
#endif