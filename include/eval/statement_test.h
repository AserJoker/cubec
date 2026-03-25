#ifndef _H_CUBEC_EVAL_STATEMENT_TEST_
#define _H_CUBEC_EVAL_STATEMENT_TEST_
#include "ast/statement_test.h"
#include "engine/context.h"
#include "engine/value.h"
#ifdef __cpluplus
extern "C" {
#endif
cubec_value_t cubec_eval_statement_test(cubec_context_t ctx,
                                        cubec_ast_statement_test_t sts,
                                        const char *filename);
#ifdef __cpluplus
}
#endif
#endif