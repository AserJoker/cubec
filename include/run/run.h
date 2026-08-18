#ifndef _H_CUBEC_RUN_RUN_
#define _H_CUBEC_RUN_RUN_
#include "engine/context.h"
#include "core/node.h"
#ifdef __cplusplus
extern "C" {
#endif

/* ---- Expression dispatcher ---- */

/**
 * @brief Run an expression node by its kind.
 * Handles all CUBEC_NODE_EXPRESSION_* and CUBEC_NODE_LITERAL_* kinds.
 */
value_t run_expression(context_t ctx, node_t node, bool shadow);

/* ---- Literal runners ---- */

value_t run_literal_numeric(context_t ctx, node_t node, bool shadow);
value_t run_literal_string(context_t ctx, node_t node, bool shadow);
value_t run_literal_char(context_t ctx, node_t node, bool shadow);
value_t run_literal_identifier(context_t ctx, node_t node, bool shadow);
value_t run_literal_nil(context_t ctx, node_t node, bool shadow);
value_t run_literal_undefined(context_t ctx, node_t node, bool shadow);

/* ---- Expression runners ---- */

value_t run_expression_binary(context_t ctx, node_t node, bool shadow);
value_t run_expression_assignment(context_t ctx, node_t node, bool shadow);
value_t run_expression_deref(context_t ctx, node_t node, bool shadow);
value_t run_expression_addr(context_t ctx, node_t node, bool shadow);
value_t run_expression_member(context_t ctx, node_t node, bool shadow);
value_t run_expression_call(context_t ctx, node_t node, bool shadow);
value_t run_expression_group(context_t ctx, node_t node, bool shadow);
value_t run_expression_subscript(context_t ctx, node_t node, bool shadow);
value_t run_expression_namespace_access(context_t ctx, node_t node,
                                        bool shadow);

/* ---- Program runner ---- */

value_t run_program(context_t ctx, node_t node, bool shadow);

#ifdef __cplusplus
}
#endif
#endif
