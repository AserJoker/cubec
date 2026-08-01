#ifndef _H_CUBEC_CUBEC_EXPRESSION_TYPE_TUPLE_
#define _H_CUBEC_CUBEC_EXPRESSION_TYPE_TUPLE_
#include "engine/context.h"
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#include "cubec/expression.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AST node for tuple type expression.
 *
 * Syntax:
 *   <type1, type2, ...>
 *
 * Used as a type expression in type aliases, parameter types, etc.
 * Example: <i32, f64> represents a tuple of i32 and f64.
 */
struct _cubec_expression_type_tuple_t;
struct _cubec_expression_type_tuple_t {
  struct _cubec_expression_t super;
  vec_t element_types; /**< Vector of type expression nodes (auto_dispose=true) */
};
typedef struct _cubec_expression_type_tuple_t *cubec_expression_type_tuple_t;

extern type_t g_cubec_expression_type_tuple_type;

struct _cubec_expression_type_tuple_init_t {
  location_t location;
  node_t parent;
  vec_t element_types;
};
typedef struct _cubec_expression_type_tuple_init_t cubec_expression_type_tuple_init_t;

/**
 * @brief Try to parse a tuple type expression.
 * @param allocator The allocator to use.
 * @param tokens The token list.
 * @param position Current position in token list (updated on success).
 * @param filename The source filename for error reporting.
 * @return A new cubec_expression_type_tuple_t node, or NULL if current token
 *         is not '<' or doesn't form a valid tuple type.
 */
node_t read_expression_type_tuple(context_t ctx, vec_t tokens,
                                    size_t *position, const char *filename);

#ifdef __cplusplus
}
#endif
#endif
