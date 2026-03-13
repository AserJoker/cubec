#ifndef _H_CUBEC_RUNTIME_EXPRESSION_CALL_
#define _H_CUBEC_RUNTIME_EXPRESSION_CALL_
#include "ast/expression_call.h"
#include "engine/context.h"
#include "engine/value.h"
#include "runtime/vm.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t cubec_run_expression_call(cubec_context_t ctx, cubec_vm_t vm,
                                        cubec_ast_expression_call_t node);
#ifdef __cplusplus
}
#endif
#endif