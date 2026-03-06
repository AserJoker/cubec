#ifndef _H_CUBEC_ASTWRITER_FUNCTION_ARGUMENT_REST_
#define _H_CUBEC_ASTWRITER_FUNCTION_ARGUMENT_REST_
#include "ast/function_argument_rest.h"
#include "core/any.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_any_t
cubec_write_ast_function_argument_rest(cubec_allocator_t allocator,
                                  cubec_ast_function_argument_rest_t self);
#ifdef __cplusplus
}
#endif
#endif