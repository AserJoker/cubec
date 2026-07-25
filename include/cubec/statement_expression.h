#ifndef _H_CUBEC_CUBEC_STATEMENT_EXPRESSION_
#define _H_CUBEC_CUBEC_STATEMENT_EXPRESSION_
#include "core/allocator.h"
#include "engine/context.h"
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _cubec_statement_expression_t;
struct _cubec_statement_expression_t {
  struct _node_t super;
  node_t expression; /**< The expression being evaluated */
};
typedef struct _cubec_statement_expression_t *cubec_statement_expression_t;

extern type_t g_cubec_statement_expression_type;

struct _cubec_statement_expression_init_t {
  location_t location;
  node_t parent;
  node_t expression;
};
typedef struct _cubec_statement_expression_init_t
    cubec_statement_expression_init_t;

/**
 * @brief Try to parse an expression statement: \c <expression> \c ";"
 *
 * An expression statement evaluates an expression for its side effects and
 * discards the result. The trailing semicolon is mandatory.
 *
 * @return A new cubec_statement_expression_t node, or NULL if parsing fails.
 */
node_t read_statement_expression(context_t ctx, vec_t tokens,
                                 size_t *position, const char *filename);

#ifdef __cplusplus
}
#endif
#endif
