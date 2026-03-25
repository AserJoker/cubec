#ifndef _H_CUBEC_EVAL_PROGRAM_
#define _H_CUBEC_EVAL_PROGRAM_
#include "ast/program.h"
#include "engine/context.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t cubec_eval_program(cubec_context_t ctx,
                                 cubec_ast_program_t program,
                                 const char *filename);
#ifdef __cplusplus
}
#endif
#endif