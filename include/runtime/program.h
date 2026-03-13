#ifndef _H_CUBEC_RUNTIME_PROGRAM_
#define _H_CUBEC_RUNTIME_PROGRAM_
#include "ast/program.h"
#include "engine/context.h"
#include "engine/value.h"
#include "runtime/vm.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t cubec_run_program(cubec_context_t ctx, cubec_vm_t vm,
                                cubec_ast_program_t node);
#ifdef __cplusplus
}
#endif
#endif