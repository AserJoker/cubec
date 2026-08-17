#ifndef _H_CUBEC_RUN_RUN_
#define _H_CUBEC_RUN_RUN_
#include "engine/context.h"
#include "core/node.h"
#ifdef __cplusplus
extern "C" {
#endif

/* ---- Literal runners ---- */

value_t run_literal_numeric(context_t ctx, node_t node, bool shadow);
value_t run_literal_string(context_t ctx, node_t node, bool shadow);
value_t run_literal_char(context_t ctx, node_t node, bool shadow);
value_t run_literal_identifier(context_t ctx, node_t node, bool shadow);
value_t run_literal_nil(context_t ctx, node_t node, bool shadow);
value_t run_literal_undefined(context_t ctx, node_t node, bool shadow);

/* ---- Program runner ---- */

value_t run_program(context_t ctx, node_t node, bool shadow);

#ifdef __cplusplus
}
#endif
#endif
