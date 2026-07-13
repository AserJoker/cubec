#ifndef _H_CUBEC_CUBEC_STATEMENT_ENUM_
#define _H_CUBEC_CUBEC_STATEMENT_ENUM_
#include "core/allocator.h"
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AST node for enum declaration statement.
 *
 * Syntax:
 *   [export] enum <name> { <items> }
 *
 * Items are enum_item nodes with syntax: <name> [: <type>] [= <value>]
 * Items are comma-separated. Type and value are both optional.
 * No generic parameters (generic needs are served by union).
 *
 * Examples:
 *   enum Color { Red, Green, Blue }
 *   enum Status { Ok: u8, Error: u8 }
 *   enum Color { Red: u8 = 0, Green: u8 = 1, Blue: u8 = 2 }
 *   export enum Color { Red, Green, Blue }
 */
struct _cubec_statement_enum_t;
struct _cubec_statement_enum_t {
  struct _node_t super;
  bool is_export;  /**< Whether this enum is exported */
  node_t name;     /**< Identifier node for the enum name */
  vec_t items;     /**< Vector of cubec_enum_item_t (auto_dispose=true) */
};
typedef struct _cubec_statement_enum_t *cubec_statement_enum_t;

extern type_t g_cubec_statement_enum_type;

struct _cubec_statement_enum_init_t {
  location_t location;
  node_t parent;
  bool is_export;
  node_t name;
  vec_t items;
};
typedef struct _cubec_statement_enum_init_t cubec_statement_enum_init_t;

/**
 * @brief Try to parse an enum declaration statement.
 * @param allocator The allocator to use.
 * @param tokens The token list.
 * @param position Current position in token list (updated on success).
 * @param filename The source filename for error reporting.
 * @return A new cubec_statement_enum_t node, or NULL if current token
 *         is not an enum declaration prefix (export/enum).
 */
node_t read_statement_enum(allocator_t allocator, vec_t tokens,
                            size_t *position, const char *filename);

#ifdef __cplusplus
}
#endif
#endif
