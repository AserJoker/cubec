#ifndef _H_CUBEC_CUBEC_LITERAL_
#define _H_CUBEC_CUBEC_LITERAL_
#include "core/allocator.h"
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _cubec_literal_t;
struct _cubec_literal_t {
  struct _node_t super;
};
typedef struct _cubec_literal_t *cubec_literal_t;

extern type_t g_cubec_literal_type;

struct _cubec_literal_init_t {
  location_t location;
  node_t parent;
};
typedef struct _cubec_literal_init_t cubec_literal_init_t;

#ifdef __cplusplus
}
#endif
#endif