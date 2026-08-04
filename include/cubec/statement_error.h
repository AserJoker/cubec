#ifndef _H_CUBEC_CUBEC_STATEMENT_ERROR_
#define _H_CUBEC_CUBEC_STATEMENT_ERROR_
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/emit_context.h"
#include "core/writer.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _cubec_statement_error_t;
struct _cubec_statement_error_t {
  struct _node_t super;
};
typedef struct _cubec_statement_error_t *cubec_statement_error_t;

extern type_t g_cubec_statement_error_type;

struct _cubec_statement_error_init_t {
  location_t location;
  node_t parent;
};
typedef struct _cubec_statement_error_init_t cubec_statement_error_init_t;

node_t create_statement_error(context_t ctx, location_t loc);

void write_statement_error(writer_t writer, node_t node);

void emit_statement_error(emit_context_t ctx, node_t node);

#ifdef __cplusplus
}
#endif
#endif
