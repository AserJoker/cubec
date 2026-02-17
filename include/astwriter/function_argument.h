#ifndef _H_CUBEC_ASTWRITER_FUNCTION_ARGUMENT_
#define _H_CUBEC_ASTWRITER_FUNCTION_ARGUMENT_
#include "ast/function_argument.h"
#include "core/value.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t
cubec_write_ast_function_argument(cubec_allocator_t allocator,
                                  cubec_ast_function_argument_t self);
#ifdef __cplusplus
}
#endif
#endif