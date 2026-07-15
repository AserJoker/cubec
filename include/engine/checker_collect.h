#ifndef _H_CUBEC_ENGINE_CHECKER_COLLECT_
#define _H_CUBEC_ENGINE_CHECKER_COLLECT_
#include "engine/checker.h"
#include "core/node.h"
#ifdef __cplusplus
extern "C" {
#endif

void checker_collect_declarations(checker_t ctx, node_t program);

#ifdef __cplusplus
}
#endif
#endif
