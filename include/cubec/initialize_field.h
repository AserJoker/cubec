#ifndef _H_CUBEC_cubec_initialize_field_
#define _H_CUBEC_cubec_initialize_field_
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#include "core/emit_context.h"
#include "core/writer.h"
#include "cubec/expression.h"
#include "cubec/literal_identifier.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _cubec_initialize_field_t;
struct _cubec_initialize_field_t {
  struct _cubec_expression_t super;
  cubec_literal_identifier_t field; /**< The field name (after '.') */
  node_t value;                     /**< The value expression (after '=') */
};
typedef struct _cubec_initialize_field_t *cubec_initialize_field_t;

extern type_t g_cubec_initialize_field_type;

struct _cubec_initialize_field_init_t {
  location_t location;
  node_t parent;
  cubec_literal_identifier_t field;
  node_t value;
};
typedef struct _cubec_initialize_field_init_t cubec_initialize_field_init_t;

/**
 * @brief Try to parse an initialize field expression: \c .identifier = \c
 * expression
 * @param allocator The allocator to use for memory allocation
 * @param tokens The token vector
 * @param position Current position in the token stream (updated on success)
 * @param filename The source filename for error reporting
 * @return A new cubec_initialize_field_t node, or NULL if the
 * current token is not \c '.' followed by identifier and \c =.
 */
node_t read_initialize_field(context_t ctx, vec_t tokens, size_t *position,
                             const char *filename);

node_t create_initialize_field(context_t ctx, location_t loc, const char *name,
                               node_t value);

void write_initialize_field(writer_t writer, node_t node);

void emit_initialize_field(emit_context_t ctx, node_t node);

#ifdef __cplusplus
}
#endif
#endif
