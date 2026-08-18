#ifndef _H_CUBEC_CUBEC_LITERAL_BOOL_
#define _H_CUBEC_CUBEC_LITERAL_BOOL_
#include "core/location.h"
#include "core/node.h"
#include "core/class.h"
#include "core/emit_context.h"
#include "cubec/literal.h"
#include "engine/vm.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _cubec_literal_bool_t {
  struct _cubec_literal_t super;
  bool value;
};
typedef struct _cubec_literal_bool_t *cubec_literal_bool_t;

extern class_t g_cubec_literal_bool_class;

node_t read_literal_bool(vm_t vm, vec_t tokens, size_t *position,
                         const char *filename);

node_t create_literal_bool(vm_t vm, location_t loc, bool value);

void emit_literal_bool(emit_context_t ctx, node_t node);

#ifdef __cplusplus
}
#endif
#endif
