#ifndef _H_CUBEC_CUBEC_STATEMENT_EMPTY_
#define _H_CUBEC_CUBEC_STATEMENT_EMPTY_
#include "core/emit_context.h"
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _cubec_statement_empty_t;
struct _cubec_statement_empty_t {
  struct _node_t super;
};
typedef struct _cubec_statement_empty_t *cubec_statement_empty_t;

extern type_t g_cubec_statement_empty_type;

struct _cubec_statement_empty_init_t {
  location_t location;
  node_t parent;
};
typedef struct _cubec_statement_empty_init_t cubec_statement_empty_init_t;

node_t read_statement_empty(context_t ctx, vec_t tokens, size_t *position,
                            const char *filename);

node_t create_statement_empty(context_t ctx, location_t loc);


void emit_statement_empty(emit_context_t ctx, node_t node);

#ifdef __cplusplus
}
#endif
#endif
