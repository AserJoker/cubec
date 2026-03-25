#ifndef _H_CUBEC_EVAL_LITERAL_IDENTIFIER_
#define _H_CUBEC_EVAL_LITERAL_IDENTIFIER_
#include "ast/literal_identifier.h"
#include "engine/context.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t
cubec_eval_literal_identifier(cubec_context_t ctx,
                              cubec_ast_literal_identifier_t identifier,
                              const char *filename);
#ifdef __cplusplus
}
#endif
#endif