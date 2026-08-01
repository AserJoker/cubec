#ifndef _H_CUBEC_CUBEC_LITERAL_UNDEFINED_
#define _H_CUBEC_CUBEC_LITERAL_UNDEFINED_
#include "core/allocator.h"
#include "engine/context.h"
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "cubec/literal.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _cubec_literal_undefined_t {
  struct _cubec_literal_t super;
};
typedef struct _cubec_literal_undefined_t *cubec_literal_undefined_t;

extern type_t g_cubec_literal_undefined_type;

node_t read_literal_undefined(context_t ctx, vec_t tokens,
                               size_t *position, const char *filename);

node_t cubec_ast_create_undefined(context_t ctx, location_t loc);

#ifdef __cplusplus
}
#endif
#endif
