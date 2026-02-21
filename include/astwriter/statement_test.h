#ifndef _H_CUBEC_ASTWRITER_STATEMENT_TEST_
#define _H_CUBEC_ASTWRITER_STATEMENT_TEST_
#include "ast/statement_test.h"
#include "core/value.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t
cubec_write_ast_statement_test(cubec_allocator_t allocator,
                               cubec_ast_statement_test_t statement);
#ifdef __cplusplus
}
#endif
#endif