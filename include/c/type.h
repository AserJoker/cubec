#ifndef _H_CUBEC_C_TYPE_
#define _H_CUBEC_C_TYPE_
#include "ast/type.h"
#include "core/string.h"
#include "engine/context.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t cubec_c_write_type(cubec_context_t self, cubec_ast_type_t type,
                                 const char *filename, cubec_string_t *output);
#ifdef __cplusplus
}
#endif
#endif