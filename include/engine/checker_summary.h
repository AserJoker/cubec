#ifndef _H_CUBEC_ENGINE_CHECKER_SUMMARY_
#define _H_CUBEC_ENGINE_CHECKER_SUMMARY_
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Post-check analysis pass: compute compilation summary.
 *
 * Scans the global scope to count types, functions, methods, and detect
 * dead code (symbols with use_count == 0 or zero instantiations).
 *
 * Called by context_check_program after all passes complete.
 * Results are stored in ctx->summary.
 */
void context_compute_summary(context_t ctx);

#ifdef __cplusplus
}
#endif
#endif
