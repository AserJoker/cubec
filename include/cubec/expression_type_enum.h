#ifndef _H_CUBEC_CUBEC_EXPRESSION_TYPE_ENUM_
#define _H_CUBEC_CUBEC_EXPRESSION_TYPE_ENUM_
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
 * @brief AST node for anonymous enum type expression.
 *
 * Syntax:
 *   enum { <items> }
 *
 * Used as a type expression in type aliases, parameter types, etc.
 * Items are enum_item nodes with syntax: <name> [: <type>] [= <value>]
 *
 * This is the expression form — it has no name and no is_export flag.
 * It inherits from expression_t so it participates in the
 * expression/type expression hierarchy.
 *
 * Examples:
 *   enum { Red, Green, Blue }
 *   enum { A: u8 = 1, B: u8 = 2 }
 *   type Flags = enum { A: u8 = 1, B: u8 = 2 }
 */
struct _cubec_expression_type_enum_t;
struct _cubec_expression_type_enum_t {
  struct _cubec_expression_t super;
  vec_t items;  /**< Vector of cubec_enum_item_t (auto_dispose=true) */
};
typedef struct _cubec_expression_type_enum_t *cubec_expression_type_enum_t;

extern type_t g_cubec_expression_type_enum_type;

struct _cubec_expression_type_enum_init_t {
  location_t location;
  node_t parent;
  vec_t items;
};
typedef struct _cubec_expression_type_enum_init_t cubec_expression_type_enum_init_t;

/**
 * @brief Try to parse an anonymous enum type expression.
 * @param allocator The allocator to use.
 * @param tokens The token list.
 * @param position Current position in token list (updated on success).
 * @param filename The source filename for error reporting.
 * @return A new cubec_expression_type_enum_t node, or NULL if current token
 *         is not 'enum' keyword.
 */
node_t read_expression_type_enum(context_t ctx, vec_t tokens,
                                  size_t *position, const char *filename);

/**
 * @brief Parse enum body after 'enum' keyword has been consumed.
 *
 * Parses { items } and returns an expression_type_enum node.
 * Used by read_statement_enum for delegation.
 *
 * @param allocator The allocator to use.
 * @param tokens The token list.
 * @param position Current position in token list (updated on success).
 * @param filename The source filename for error reporting.
 * @param start_location Location of the 'enum' keyword (for error span).
 * @return A new cubec_expression_type_enum_t node, or NULL on error.
 */
node_t read_expression_type_enum_body(context_t ctx, vec_t tokens,
                                       size_t *position, const char *filename,
                                       location_t start_location);

node_t create_expression_type_enum(context_t ctx, location_t loc, vec_t items);

#ifdef __cplusplus
}
#endif
#endif
