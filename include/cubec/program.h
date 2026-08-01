#ifndef _H_CUBEC_CUBEC_PROGRAM_
#define _H_CUBEC_CUBEC_PROGRAM_
#include "engine/context.h"
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#ifdef __cplusplus
extern "C" {
#endif
struct _cubec_program_node_t;
struct _cubec_program_node_t {
  struct _node_t super;
  vec_t statements;
};
typedef struct _cubec_program_node_t *cubec_program_node_t;
extern type_t g_cubec_program_node_type;
struct _cubec_program_node_init_t {
  location_t location;
  node_t parent;
  vec_t statements; /**< If non-NULL, ownership is transferred; if NULL, an empty vec is created */
};
typedef struct _cubec_program_node_init_t cubec_program_node_init_t;
node_t read_program_node(context_t ctx, vec_t tokens, size_t *position,
                         const char *filename);
node_t cubec_ast_create_program(context_t ctx, location_t loc, vec_t statements);

#ifdef __cplusplus
}
#endif
#endif