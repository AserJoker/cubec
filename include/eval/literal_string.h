#ifndef _H_CUBEC_EVAL_LITERAL_STRING_
#define _H_CUBEC_EVAL_LITERAL_STRING_
#include "ast/literal_string.h"
#include "engine/context.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t cubec_eval_literal_string(cubec_context_t ctx,
                                        cubec_ast_literal_string_t str,
                                        const char *filename);
#ifdef __cplusplus
}
#endif
#endif