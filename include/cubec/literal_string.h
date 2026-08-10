#ifndef _H_CUBEC_CUBEC_LITERAL_STRING_
#define _H_CUBEC_CUBEC_LITERAL_STRING_
#include "core/location.h"
#include "core/node.h"
#include "core/string.h"
#include "core/class.h"
#include "core/vec.h"
#include "core/emit_context.h"
#include "cubec/literal.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _cubec_literal_string_t;
struct _cubec_literal_string_t {
  struct _cubec_literal_t super;
  string_t value;
};
typedef struct _cubec_literal_string_t *cubec_literal_string_t;

extern class_t g_cubec_literal_string_class;

struct _cubec_literal_string_init_t {
  location_t location;
  node_t parent;
  const char *value;
};
typedef struct _cubec_literal_string_init_t cubec_literal_string_init_t;

node_t read_literal_string(context_t ctx, vec_t tokens, size_t *position,
                           const char *filename);

node_t create_literal_string(context_t ctx, location_t loc, const char *value);


void emit_literal_string(emit_context_t ctx, node_t node);

#ifdef __cplusplus
}
#endif
#endif