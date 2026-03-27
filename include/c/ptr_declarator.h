#ifndef _H_CUBEC_C_PTR_DECLARATOR_
#define _H_CUBEC_C_PTR_DECLARATOR_
#include "ast/ptr_declarator.h"
#include "core/string.h"
#include "engine/context.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t cubec_c_write_ptr_declarator(cubec_context_t self,
                                           cubec_ast_node_t dec,
                                           const char *filename,
                                           cubec_string_t *output);
#ifdef __cplusplus
}
#endif
#endif