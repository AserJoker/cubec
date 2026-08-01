#ifndef _H_CUBEC_CUBEC_ENUM_ITEM_
#define _H_CUBEC_CUBEC_ENUM_ITEM_
#include "engine/context.h"
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AST node for an enum item (member of an enum).
 *
 * Syntax:
 *   <identifier> [: <type>] [= <value>]
 *
 * Type and value are both optional. When type is omitted, it is inferred.
 * When value is omitted, it auto-increments from the previous item.
 *
 * Examples:
 *   Red
 *   Ok: u8
 *   Red: u8 = 0
 *   Green = 1
 */
struct _cubec_enum_item_t;
struct _cubec_enum_item_t {
  struct _node_t super;
  node_t name;   /**< Identifier node for the item name (required) */
  node_t type;   /**< Type expression (nullable, inferred when omitted) */
  node_t value;  /**< Value expression (nullable, auto-increment when omitted) */
};
typedef struct _cubec_enum_item_t *cubec_enum_item_t;

extern type_t g_cubec_enum_item_type;

struct _cubec_enum_item_init_t {
  location_t location;
  node_t parent;
  node_t name;
  node_t type;
  node_t value;
};
typedef struct _cubec_enum_item_init_t cubec_enum_item_init_t;

/**
 * @brief Try to parse an enum item: <identifier> [: <type>] [= <value>]
 * @param allocator The allocator to use.
 * @param tokens The token list.
 * @param position Current position in token list (updated on success).
 * @param filename The source filename for error reporting.
 * @return A new cubec_enum_item_t node, or NULL if current tokens
 *         don't match the item pattern.
 */
node_t read_enum_item(context_t ctx, vec_t tokens,
                       size_t *position, const char *filename);

node_t cubec_ast_create_enum_item(context_t ctx, location_t loc, const char *name, node_t type, node_t value);

#ifdef __cplusplus
}
#endif
#endif
