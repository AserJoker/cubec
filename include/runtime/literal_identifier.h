#ifndef _H_CUBEC_RUNTIME_LITERAL_IDENTIFIER_
#define _H_CUBEC_RUNTIME_LITERAL_IDENTIFIER_
#include "ast/literal_identifier.h"
#include "engine/context.h"
#include "engine/value.h"
#include "runtime/vm.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t cubec_run_literal_identifier(cubec_context_t ctx, cubec_vm_t vm,
                                           cubec_ast_literal_identifier_t node);
#ifdef __cplusplus
}
#endif
#endif