#ifndef _H_CUBEC_ENGINE_CHECKER_COLLECT_
#define _H_CUBEC_ENGINE_CHECKER_COLLECT_
#include "engine/context.h"
#include "core/node.h"
#ifdef __cplusplus
extern "C" {
#endif

void context_collect_declarations(context_t ctx, node_t program);
void context_collect_statement(context_t ctx, node_t stmt);

#ifdef __cplusplus
}
#endif
#endif
