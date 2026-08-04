#ifndef _H_CUBEC_CUBEC_EXPRESSION_WILDCARD_
#define _H_CUBEC_CUBEC_EXPRESSION_WILDCARD_
#include "core/emit_context.h"
#include "core/writer.h"
#include "cubec/expression.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AST node for wildcard type expression `?` or `<?>`.
 *
 * Used in constraint types to skip partial checks:
 *   - []? — only check slice structure, ignore element type
 *   - *? — only check pointer structure, ignore pointee type
 *   - Vec[?] — only check Vec template, ignore type args
 *   - <?> — T must be a tuple type (is_tuple = true)
 *
 * Syntax: `?` (single question mark in atom position) or `<?>` (tuple wildcard)
 */
struct _cubec_expression_wildcard_t;
struct _cubec_expression_wildcard_t {
  struct _cubec_expression_t super;
  bool is_tuple; /**< true when parsed as <?>, false for standalone ? */
};
typedef struct _cubec_expression_wildcard_t *cubec_expression_wildcard_t;

extern type_t g_cubec_expression_wildcard_type;

struct _cubec_expression_wildcard_init_t {
  location_t location;
  node_t parent;
  bool is_tuple;
};
typedef struct _cubec_expression_wildcard_init_t cubec_expression_wildcard_init_t;

/**
 * @brief Parse a wildcard type expression `?`.
 * @return Wildcard node, or NULL if current token is not `?`.
 */
node_t read_expression_wildcard(context_t ctx, vec_t tokens,
                                size_t *position, const char *filename);

node_t create_expression_wildcard(context_t ctx, location_t loc, bool is_tuple);

void write_expression_wildcard(writer_t writer, node_t node);

void emit_expression_wildcard(emit_context_t ctx, node_t node);

#ifdef __cplusplus
}
#endif
#endif
