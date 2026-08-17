#ifndef _H_CUBEC_ENGINE_RUNNER_
#define _H_CUBEC_ENGINE_RUNNER_
#include "engine/type.h"
#include "core/node.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Run a program AST node: execute each top-level statement.
 *
 * @param vm   VM instance (scope, types, etc.)
 * @param node AST node (must be PROGRAM kind)
 * @return void value on success, error/interrupt on failure
 */
value_t run_program(vm_t vm, node_t node);

#ifdef __cplusplus
}
#endif
#endif
