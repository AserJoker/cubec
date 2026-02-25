#ifndef _H_CUBEC_ENGINE_INTERFACE_
#define _H_CUBEC_ENGINE_INTERFACE_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/map.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_interface_type_t {
  struct _cubec_type_t super;
  cubec_type_t return_type;
  cubec_array_t args;
  cubec_map_t closure;
  bool variadic;
} *cubec_interface_type_t;

typedef struct _cubec_interface_value_t {
  struct _cubec_value_t super;
  cubec_map_t closure;
  cubec_ast_node_t *node;
} *cubec_interface_value_t;
cubec_type_t cubec_create_interface_type(cubec_allocator_t allocator,
                                         const char *name,
                                         cubec_type_t return_type,
                                         bool variadic);
cubec_value_t cubec_create_interface_value(cubec_allocator_t allocator,
                                           cubec_type_t type,
                                           cubec_ast_node_t *node);
#ifdef __cplusplus
}
#endif
#endif