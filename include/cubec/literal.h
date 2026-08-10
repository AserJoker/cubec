#ifndef _H_CUBEC_CUBEC_LITERAL_
#define _H_CUBEC_CUBEC_LITERAL_
#include "core/location.h"
#include "core/class.h"
#include "cubec/expression.h"
#include "cubec/node.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _cubec_literal_t;
struct _cubec_literal_t {
  struct _cubec_expression_t super;
};
typedef struct _cubec_literal_t *cubec_literal_t;

extern class_t g_cubec_literal_class;

struct _cubec_literal_init_t {
  location_t location;
  node_t parent;
  cubec_node_kind_t kind;
};
typedef struct _cubec_literal_init_t cubec_literal_init_t;

#ifdef __cplusplus
}
#endif
#endif
