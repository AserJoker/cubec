#ifndef _H_CUBEC_CUBEC_EXPRESSION_TYPE_CONSTRAINT_
#define _H_CUBEC_CUBEC_EXPRESSION_TYPE_CONSTRAINT_
#include "core/allocator.h"
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#include "cubec/expression.h"
#ifdef __cplusplus
extern "C" {
#endif

/** @brief Operator kind for type constraint expressions. */
enum _cubec_type_constraint_operator_t {
  CUBEC_TYPE_CONSTRAINT_EXTENDS, /**< T extends U  — subtype check */
  CUBEC_TYPE_CONSTRAINT_EQ,      /**< T == U       — type equality */
  CUBEC_TYPE_CONSTRAINT_NE,      /**< T != U       — type inequality */
};
typedef enum _cubec_type_constraint_operator_t cubec_type_constraint_operator_t;

struct _cubec_expression_type_constraint_t;
struct _cubec_expression_type_constraint_t {
  struct _cubec_expression_t super;
  cubec_type_constraint_operator_t op; /**< extends / == / != */
  node_t left;                         /**< Left operand type expression */
  node_t right;                        /**< Right operand type expression */
};
typedef struct _cubec_expression_type_constraint_t *cubec_expression_type_constraint_t;

extern type_t g_cubec_expression_type_constraint_type;

struct _cubec_expression_type_constraint_init_t {
  location_t location;
  node_t parent;
  cubec_type_constraint_operator_t op;
  node_t left;
  node_t right;
};
typedef struct _cubec_expression_type_constraint_init_t cubec_expression_type_constraint_init_t;

/**
 * @brief Try to parse a type constraint expression:
 *        left_type (extends | == | !=) right_type
 *
 * Parsing strategy:
 * 1. Parse left operand via read_type_expression_primary
 * 2. Check for constraint operator keyword
 * 3. Parse right operand via read_expression_type
 *
 * If left exists but no operator follows, returns the left operand directly
 * (graceful fallback — the caller is responsible for detecting whether a
 * ternary '?' follows).
 *
 * @return A new cubec_expression_type_constraint_t node, or the left operand
 *         as-is if no constraint operator was found.
 */
node_t read_expression_type_constraint(allocator_t allocator, vec_t tokens,
                                       size_t *position, const char *filename);

#ifdef __cplusplus
}
#endif
#endif
