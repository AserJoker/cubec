#ifndef _H_CUBEC_RUNTIME_EXPRESSION_BINARY_
#define _H_CUBEC_RUNTIME_EXPRESSION_BINARY_
#include "ast/expression_binary.h"
#include "engine/context.h"
#include "engine/value.h"
#include "runtime/vm.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t cubec_run_expression_binary(cubec_context_t ctx, cubec_vm_t vm,
                                          cubec_ast_expression_binary_t node);
#ifdef __cplusplus
}
#endif
#endif