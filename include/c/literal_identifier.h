#ifndef _H_CUBEC_C_LITERAL_IDENTIFIER_
#define _H_CUBEC_C_LITERAL_IDENTIFIER_
#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "core/string.h"
#include "engine/context.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t cubec_c_write_literal_identifier(cubec_context_t self,
                                               cubec_ast_node_t identifier,
                                               const char *filename,
                                               cubec_string_t *output);
#ifdef __cplusplus
}
#endif
#endif