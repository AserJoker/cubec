#ifndef _H_CUBEC_CUBEC_DECLARATION_
#define _H_CUBEC_CUBEC_DECLARATION_
#include "core/allocator.h"
#include "core/location.h"
#include "core/type.h"
#include "core/vec.h"
#include "cubec/expression.h"
#include "cubec/node.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _cubec_declaration_t;
struct _cubec_declaration_t {
  struct _cubec_expression_t super;
};
typedef struct _cubec_declaration_t *cubec_declaration_t;

extern type_t g_cubec_declaration_type;

struct _cubec_declaration_init_t {
  location_t location;
  node_t parent;
  cubec_node_kind_t kind;
};
typedef struct _cubec_declaration_init_t cubec_declaration_init_t;

#ifdef __cplusplus
}
#endif
#endif