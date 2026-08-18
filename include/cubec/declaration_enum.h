#ifndef _H_CUBEC_CUBEC_DECLARATION_ENUM_
#define _H_CUBEC_CUBEC_DECLARATION_ENUM_
#include "engine/vm.h"
#include "core/location.h"
#include "core/node.h"
#include "core/class.h"
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
struct _cubec_declaration_enum_t;
struct _cubec_declaration_enum_t {
  struct _cubec_expression_t super;
  vec_t items;  /**< Vector of cubec_enum_item_t (auto_dispose=true) */
};
typedef struct _cubec_declaration_enum_t *cubec_declaration_enum_t;

extern class_t g_cubec_declaration_enum_class;

struct _cubec_declaration_enum_init_t {
  location_t location;
  node_t parent;
  vec_t items;
};
typedef struct _cubec_declaration_enum_init_t cubec_declaration_enum_init_t;

/**
 * @brief Try to parse an anonymous enum type expression.
 * @param allocator The allocator to use.
 * @param tokens The token list.
 * @param position Current position in token list (updated on success).
 * @param filename The source filename for error reporting.
 * @return A new cubec_declaration_enum_t node, or NULL if current token
 *         is not 'enum' keyword.
 */
node_t read_declaration_enum(vm_t vm, vec_t tokens,
                                  size_t *position, const char *filename);

/**
 * @brief Parse enum body after 'enum' keyword has been consumed.
 *
 * Parses { items } and returns an declaration_enum node.
 * Used by read_statement_enum for delegation.
 *
 * @param allocator The allocator to use.
 * @param tokens The token list.
 * @param position Current position in token list (updated on success).
 * @param filename The source filename for error reporting.
 * @param start_location Location of the 'enum' keyword (for error span).
 * @return A new cubec_declaration_enum_t node, or NULL on error.
 */
node_t read_declaration_enum_body(vm_t vm, vec_t tokens,
                                       size_t *position, const char *filename,
                                       location_t start_location);

node_t create_declaration_enum(vm_t vm, location_t loc, vec_t items);


void emit_declaration_enum(emit_context_t ctx, node_t node);

#ifdef __cplusplus
}
#endif
#endif
