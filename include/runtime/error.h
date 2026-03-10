#ifndef _H_CUBEC_RUNTIME_ERROR_
#define _H_CUBEC_RUNTIME_ERROR_
#include "ast/node.h"
#include "engine/context.h"
#include "engine/value.h"
#include "runtime/vm.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t cubec_run_error_node(cubec_context_t ctx, cubec_vm_t vm,
                                   cubec_ast_error_t node);
#ifdef __cplusplus
}
#endif
#endif