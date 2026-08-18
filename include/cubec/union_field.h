#ifndef _H_CUBEC_CUBEC_UNION_FIELD_
#define _H_CUBEC_CUBEC_UNION_FIELD_
#include "core/location.h"
#include "core/node.h"
#include "core/class.h"
#include "core/vec.h"
#include "core/emit_context.h"
#include "engine/vm.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AST node for a union field.
 *
 * Syntax:
 *   <identifier> : <type> ;
 *
 * Union fields define the variants of a tagged union.
 * Fields are semicolon-separated (consistent with struct fields).
 * No 'pub' modifier (union fields are always visible).
 *
 * Examples:
 *   value: T
 *   tag: u64
 *   err: E
 */
struct _cubec_union_field_t;
struct _cubec_union_field_t {
  struct _node_t super;
  node_t name; /**< Identifier node for the field name */
  node_t type; /**< Type expression for the field type */
};
typedef struct _cubec_union_field_t *cubec_union_field_t;

extern class_t g_cubec_union_field_class;

struct _cubec_union_field_init_t {
  location_t location;
  node_t parent;
  node_t name;
  node_t type;
};
typedef struct _cubec_union_field_init_t cubec_union_field_init_t;

/**
 * @brief Try to parse a union field: <identifier> : <type> ;
 * @param allocator The allocator to use.
 * @param tokens The token list.
 * @param position Current position in token list (updated on success).
 * @param filename The source filename for error reporting.
 * @return A new cubec_union_field_t node, or NULL if current tokens
 *         don't match the field pattern.
 */
node_t read_union_field(vm_t vm, vec_t tokens, size_t *position,
                        const char *filename);

node_t create_union_field(vm_t vm, location_t loc, const char *name,
                          node_t type);


void emit_union_field(emit_context_t ctx, node_t node);

#ifdef __cplusplus
}
#endif
#endif
