#ifndef _H_CUBEC_CUBEC_EXPRESSION_WILDCARD_
#define _H_CUBEC_CUBEC_EXPRESSION_WILDCARD_
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

/**
 * @brief Parse a wildcard type expression `?`.
 * @return Wildcard node, or NULL if current token is not `?`.
 */
node_t read_expression_wildcard(context_t ctx, vec_t tokens,
                                size_t *position, const char *filename);

#ifdef __cplusplus
}
#endif
#endif
