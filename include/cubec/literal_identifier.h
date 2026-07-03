#ifndef _H_CUBEC_CUBEC_LITERAL_IDENTIFIER_
#define _H_CUBEC_CUBEC_LITERAL_IDENTIFIER_
#include "core/allocator.h"
#include "core/location.h"
#include "core/node.h"
#include "core/string.h"
#include "core/type.h"
#include "core/vec.h"
#include "cubec/literal.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _cubec_literal_identifier_t;
struct _cubec_literal_identifier_t {
  struct _cubec_literal_t super;
  string_t value;
};
typedef struct _cubec_literal_identifier_t *cubec_literal_identifier_t;

extern type_t g_cubec_literal_identifier_type;

struct _cubec_literal_identifier_init_t {
  location_t location;
  node_t parent;
  const char *value;
};
typedef struct _cubec_literal_identifier_init_t cubec_literal_identifier_init_t;

node_t read_literal_identifier(allocator_t allocator, vec_t tokens, size_t *position,
                               const char *filename);

#ifdef __cplusplus
}
#endif
#endif