#ifndef _H_CUBEC_CUBEC_STRUCT_FIELD_
#define _H_CUBEC_CUBEC_STRUCT_FIELD_
#include "core/allocator.h"
#include "engine/context.h"
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AST node for a struct instance field.
 *
 * Syntax:
 *   [pub] <identifier> : <type> ;
 *
 * Instance fields define the memory layout of a struct.
 * The `pub` modifier controls field visibility (default = private).
 *
 * This is distinct from `var` declarations inside a struct body,
 * which are static/class-level fields.
 *
 * Examples:
 *   x: f64;
 *   pub name: *u8;
 *   data: *T;
 */
struct _cubec_struct_field_t;
struct _cubec_struct_field_t {
  struct _node_t super;
  bool is_pub;    /**< Whether this field is public */
  node_t name;    /**< Identifier node for the field name */
  node_t type;    /**< Type expression for the field type */
};
typedef struct _cubec_struct_field_t *cubec_struct_field_t;

extern type_t g_cubec_struct_field_type;

struct _cubec_struct_field_init_t {
  location_t location;
  node_t parent;
  bool is_pub;
  node_t name;
  node_t type;
};
typedef struct _cubec_struct_field_init_t cubec_struct_field_init_t;

/**
 * @brief Try to parse a struct instance field: [pub] <identifier> : <type> ;
 * @param allocator The allocator to use.
 * @param tokens The token list.
 * @param position Current position in token list (updated on success).
 * @param filename The source filename for error reporting.
 * @return A new cubec_struct_field_t node, or NULL if current tokens
 *         don't match the field pattern.
 */
node_t read_struct_field(context_t ctx, vec_t tokens,
                          size_t *position, const char *filename);

#ifdef __cplusplus
}
#endif
#endif
