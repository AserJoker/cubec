#ifndef _H_CUBEC_RUNTIME_STATEMENT_EXPRESSION_
#define _H_CUBEC_RUNTIME_STATEMENT_EXPRESSION_
#include "ast/statement_expression.h"
#include "engine/context.h"
#include "engine/value.h"
#include "runtime/vm.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t
cubec_run_statement_expression(cubec_context_t ctx, cubec_vm_t vm,
                               cubec_ast_statement_expression_t node);
#ifdef __cplusplus
}
#endif
#endif