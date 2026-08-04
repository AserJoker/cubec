#ifndef _H_CUBEC_CUBEC_ERROR_
#define _H_CUBEC_CUBEC_ERROR_
#include "core/emit_context.h"
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "cubec/node.h"
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif

struct _cubec_node_error_t;
struct _cubec_node_error_t {
  struct _node_t super;
};
typedef struct _cubec_node_error_t *cubec_node_error_t;

extern type_t g_cubec_node_error_type;

struct _cubec_node_error_init_t {
  location_t location;
  node_t parent;
};
typedef struct _cubec_node_error_init_t cubec_node_error_init_t;

static inline bool node_is_error(node_t node) {
  return node && (node->kind == CUBEC_NODE_ERROR ||
                  node->kind == CUBEC_NODE_STATEMENT_ERROR);
}

static inline bool node_is_ok(node_t node) {
  return node && !node_is_error(node);
}

node_t create_error(context_t ctx, location_t loc);

void emit_node_error(emit_context_t ctx, node_t node);

#ifdef __cplusplus
}
#endif
#endif
