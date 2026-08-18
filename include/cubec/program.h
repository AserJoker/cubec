#ifndef _H_CUBEC_CUBEC_PROGRAM_
#define _H_CUBEC_CUBEC_PROGRAM_
#include "core/location.h"
#include "core/node.h"
#include "core/class.h"
#include "core/vec.h"
#include "core/emit_context.h"
#include "engine/vm.h"
#ifdef __cplusplus
extern "C" {
#endif
struct _cubec_program_node_t;
struct _cubec_program_node_t {
  struct _node_t super;
  vec_t statements;
};
typedef struct _cubec_program_node_t *cubec_program_node_t;
extern class_t g_cubec_program_node_class;
struct _cubec_program_node_init_t {
  location_t location;
  node_t parent;
  vec_t statements; /**< If non-NULL, ownership is transferred; if NULL, an
                       empty vec is created */
};
typedef struct _cubec_program_node_init_t cubec_program_node_init_t;
node_t read_program_node(vm_t vm, vec_t tokens, size_t *position,
                         const char *filename);

node_t create_program(vm_t vm, location_t loc, vec_t statements);


void emit_program(emit_context_t ctx, node_t node);

#ifdef __cplusplus
}
#endif
#endif