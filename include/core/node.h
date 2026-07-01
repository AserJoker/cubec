#ifndef _H_CUBEC_CORE_NODE_
#define _H_CUBEC_CORE_NODE_
#include "core/allocator.h"
#include "core/location.h"
#include "core/type.h"
#include <stdbool.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
struct _node_t;
typedef struct _node_t *node_t;
struct _node_t {
  allocator_t allocator;
  uint32_t kind;
  location_t location;
  node_t parent;
};
extern type_t g_node_type;
struct _node_init_t {
  uint32_t kind;
  location_t location;
  node_t parent;
};
typedef struct _node_init_t node_init_t;
#ifdef __cplusplus
}
#endif
#endif