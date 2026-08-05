#ifndef _H_CUBEC_ENGINE_NAME_COLLECTOR_
#define _H_CUBEC_ENGINE_NAME_COLLECTOR_
#include "engine/context.h"
#include "engine/module.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Run name collection on a module.
 *
 * Traverses the module's program AST, registering declaration names into the
 * module's root_scope. For static scopes (module, type), all declaration names
 * are collected (hoisted). For non-static scopes (function, block), only
 * function and type names are collected — variables are not hoisted.
 *
 * When encountering an import statement, the dependency module is processed
 * first (recursive name collection), and its exported names are registered
 * as NAME_NAMESPACE entries.
 */
void name_collector_run(context_t ctx, module_t mod);

#ifdef __cplusplus
}
#endif
#endif /* _H_CUBEC_ENGINE_NAME_COLLECTOR_ */
