#ifndef _H_CUBEC_ENGINE_DEF_COLLECTOR_
#define _H_CUBEC_ENGINE_DEF_COLLECTOR_

#include "engine/context.h"
#include "engine/module.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Run definition collection on a module.
 *
 * Traverses the module's AST statements, creates def objects for each
 * declaration, and binds them to the corresponding name_t entries in
 * the module's root scope. Handles import/export recursively.
 */
void def_collector_run(context_t ctx, module_t mod);

#ifdef __cplusplus
}
#endif

#endif /* _H_CUBEC_ENGINE_DEF_COLLECTOR_ */
