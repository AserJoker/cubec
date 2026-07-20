#ifndef _H_CUBEC_ENGINE_FLOW_STATE_
#define _H_CUBEC_ENGINE_FLOW_STATE_
#include "core/allocator.h"
#include "core/vec.h"
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Control flow termination state for a path.
 */
enum flow_termination {
  FLOW_ALIVE,       /**< Path is normally reachable */
  FLOW_RETURNED,    /**< Path encountered a return statement */
  FLOW_BROKE,       /**< Path encountered a break statement */
  FLOW_CONTINUED,   /**< Path encountered a continue statement */
};

/**
 * @brief Flow state tracks reachability and TDZ variables along a control flow path.
 *        Each path through the AST gets its own flow_state; branches are merged
 *        using union semantics for TDZ sets.
 */
struct flow_state {
  enum flow_termination term;  /**< Termination state of this path */
  vec_t tdz_set;               /**< vec of const char* — variables still in TDZ on this path */
};

typedef struct flow_state *flow_state_t;

/* --- lifecycle --- */

/** @brief Create a flow state with FLOW_ALIVE and empty TDZ set. */
flow_state_t flow_state_create(allocator_t allocator);

/** @brief Dispose a flow state. */
void flow_state_dispose(flow_state_t fs, allocator_t allocator);

/** @brief Clone a flow state. */
flow_state_t flow_state_clone(allocator_t allocator, flow_state_t src);

/* --- termination --- */

/** @brief Create a FLOW_ALIVE state with empty TDZ set (convenience). */
flow_state_t flow_state_alive(allocator_t allocator);

void flow_state_mark_returned(flow_state_t fs);
void flow_state_mark_broke(flow_state_t fs);
void flow_state_mark_continued(flow_state_t fs);

/* --- TDZ --- */

void flow_state_add_tdz(flow_state_t fs, const char *name);
void flow_state_remove_tdz(flow_state_t fs, const char *name);
bool flow_state_is_tdz(flow_state_t fs, const char *name);

/* --- merge --- */

/**
 * @brief Merge two flow states (union semantics for TDZ).
 *        If either path is FLOW_ALIVE, result is FLOW_ALIVE.
 *        Both RETURNED => RETURNED.
 *        TDZ sets are unioned (variable is TDZ if it's TDZ in either branch).
 */
flow_state_t flow_state_merge(allocator_t allocator,
                               flow_state_t a, flow_state_t b);

/* --- queries --- */

bool flow_state_is_unreachable(flow_state_t fs);
bool flow_state_is_all_returned(flow_state_t fs);

#ifdef __cplusplus
}
#endif
#endif
