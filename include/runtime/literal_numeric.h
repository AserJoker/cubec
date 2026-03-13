#ifndef _H_CUBEC_RUNTIME_LITERAL_NUMERIC_
#define _H_CUBEC_RUNTIME_LITERAL_NUMERIC_
#include "ast/literal_numeric.h"
#include "engine/context.h"
#include "engine/value.h"
#include "runtime/vm.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t cubec_run_literal_numeric(cubec_context_t ctx, cubec_vm_t vm,
                                        cubec_ast_literal_numeric_t node);
#ifdef __cplusplus
}
#endif
#endif