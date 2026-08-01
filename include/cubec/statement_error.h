#ifndef _H_CUBEC_CUBEC_STATEMENT_ERROR_
#define _H_CUBEC_CUBEC_STATEMENT_ERROR_
#include "core/allocator.h"
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
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

node_t cubec_ast_create_error_stmt(context_t ctx, location_t loc);

#ifdef __cplusplus
}
#endif
#endif
