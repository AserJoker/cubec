#ifndef _H_CUBEC_CUBEC_LITERAL_CHAR_
#define _H_CUBEC_CUBEC_LITERAL_CHAR_
#include "engine/context.h"
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#include "cubec/literal.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _cubec_literal_char_t;
struct _cubec_literal_char_t {
  struct _cubec_literal_t super;
  char value;
};
typedef struct _cubec_literal_char_t *cubec_literal_char_t;

extern type_t g_cubec_literal_char_type;

struct _cubec_literal_char_init_t {
  location_t location;
  node_t parent;
  char value;
};
typedef struct _cubec_literal_char_init_t cubec_literal_char_init_t;

node_t read_literal_char(context_t ctx, vec_t tokens, size_t *position,
                         const char *filename);

node_t cubec_ast_create_char(context_t ctx, location_t loc, char value);

#ifdef __cplusplus
}
#endif
#endif