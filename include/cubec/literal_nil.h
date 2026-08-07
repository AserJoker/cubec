#ifndef _H_CUBEC_CUBEC_LITERAL_NIL_
#define _H_CUBEC_CUBEC_LITERAL_NIL_
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/emit_context.h"
#include "cubec/literal.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _cubec_literal_nil_t {
  struct _cubec_literal_t super;
};
typedef struct _cubec_literal_nil_t *cubec_literal_nil_t;

extern type_t g_cubec_literal_nil_type;

node_t read_literal_nil(context_t ctx, vec_t tokens, size_t *position,
                        const char *filename);

node_t create_literal_nil(context_t ctx, location_t loc);

void emit_literal_nil(emit_context_t ctx, node_t node);

#ifdef __cplusplus
}
#endif
#endif
