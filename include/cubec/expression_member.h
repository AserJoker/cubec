#ifndef _H_CUBEC_CUBEC_EXPRESSION_MEMBER_
#define _H_CUBEC_CUBEC_EXPRESSION_MEMBER_
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

struct _cubec_expression_member_t;
struct _cubec_expression_member_t {
  struct _cubec_expression_t super;
  node_t host;                      /**< The left-hand expression */
  cubec_literal_identifier_t field; /**< The field name (identifier literal) */
};
typedef struct _cubec_expression_member_t *cubec_expression_member_t;

extern type_t g_cubec_expression_member_type;

struct _cubec_expression_member_init_t {
  location_t location;
  node_t parent;
  node_t host;
  cubec_literal_identifier_t field;
};
typedef struct _cubec_expression_member_init_t cubec_expression_member_init_t;

/**
 * @brief Try to parse a member access expression: \c .identifier
 * @param host  The already-parsed left-hand expression (from read_value).
 * @return A new cubec_expression_member_t node, or NULL if the current
 *         token is not \c '.'.
 */
node_t read_expression_member(context_t ctx, vec_t tokens, size_t *position,
                              const char *filename, node_t host);

node_t create_expression_member(context_t ctx, location_t loc, node_t host,
                                const char *field);

void write_expression_member(writer_t writer, node_t node);

void emit_expression_member(emit_context_t ctx, node_t node);

#ifdef __cplusplus
}
#endif
#endif
