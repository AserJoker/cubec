#ifndef _H_CUBEC_CUBEC_LITERAL_NUMERIC_
#define _H_CUBEC_CUBEC_LITERAL_NUMERIC_
#include "core/location.h"
#include "core/node.h"
#include "core/string.h"
#include "core/type.h"
#include "core/vec.h"
#include "core/writer.h"
#include "cubec/literal.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif

enum _cubec_literal_numeric_kind_t {
  CUBEC_LITERAL_NUMERIC_KIND_INTEGER,
  CUBEC_LITERAL_NUMERIC_KIND_FLOAT,
};
typedef enum _cubec_literal_numeric_kind_t cubec_literal_numeric_kind_t;

enum _cubec_literal_numeric_type_t {
  CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT,
  CUBEC_LITERAL_NUMERIC_TYPE_I8,
  CUBEC_LITERAL_NUMERIC_TYPE_I16,
  CUBEC_LITERAL_NUMERIC_TYPE_I32,
  CUBEC_LITERAL_NUMERIC_TYPE_I64,
  CUBEC_LITERAL_NUMERIC_TYPE_U8,
  CUBEC_LITERAL_NUMERIC_TYPE_U16,
  CUBEC_LITERAL_NUMERIC_TYPE_U32,
  CUBEC_LITERAL_NUMERIC_TYPE_U64,
  CUBEC_LITERAL_NUMERIC_TYPE_F16,
  CUBEC_LITERAL_NUMERIC_TYPE_F32,
  CUBEC_LITERAL_NUMERIC_TYPE_F64,
};
typedef enum _cubec_literal_numeric_type_t cubec_literal_numeric_type_t;

struct _cubec_literal_numeric_t;
struct _cubec_literal_numeric_t {
  struct _cubec_literal_t super;
  cubec_literal_numeric_kind_t kind;
  cubec_literal_numeric_type_t numeric_type;
  string_t value;
};
typedef struct _cubec_literal_numeric_t *cubec_literal_numeric_t;

extern type_t g_cubec_literal_numeric_type;

struct _cubec_literal_numeric_init_t {
  location_t location;
  node_t parent;
  const char *value;
  cubec_literal_numeric_kind_t kind;
  cubec_literal_numeric_type_t numeric_type;
};
typedef struct _cubec_literal_numeric_init_t cubec_literal_numeric_init_t;

node_t read_literal_numeric(context_t ctx, vec_t tokens, size_t *position,
                            const char *filename);

const char *
cubec_literal_numeric_type_to_string(cubec_literal_numeric_type_t type);

node_t create_literal_numeric(context_t ctx, location_t loc, const char *value,
                              cubec_literal_numeric_kind_t kind,
                              cubec_literal_numeric_type_t ntype);

void write_literal_numeric(writer_t writer, node_t node);

#ifdef __cplusplus
}
#endif
#endif