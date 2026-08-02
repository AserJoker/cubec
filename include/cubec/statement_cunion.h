#ifndef _H_CUBEC_CUBEC_STATEMENT_CUNION_
#define _H_CUBEC_CUBEC_STATEMENT_CUNION_
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AST node for cunion (C-style union) declaration statement.
 *
 * Syntax:
 *   cunion <name> { <fields> }
 *
 * Fields use semicolon separator, same as struct instance fields:
 *   <identifier> : <type> ;
 *
 * No export, no generic parameters, no anonymous type expression.
 * C-style union: fields overlap in memory, no tag byte.
 *
 * Examples:
 *   cunion Data { int_val: i32; float_val: f64; }
 *   cunion Value { as_i32: i32; as_f64: f64; as_ptr: *u8; }
 */
struct _cubec_statement_cunion_t;
struct _cubec_statement_cunion_t {
  struct _node_t super;
  node_t name;      /**< Identifier node for the cunion name */
  vec_t fields;     /**< Vector of cubec_struct_field_t (auto_dispose=true) */
  vec_t decorators; /**< Vector of cubec_decorator_t (may be NULL) */
};
typedef struct _cubec_statement_cunion_t *cubec_statement_cunion_t;

extern type_t g_cubec_statement_cunion_type;

struct _cubec_statement_cunion_init_t {
  location_t location;
  node_t parent;
  node_t name;
  vec_t fields;
  vec_t decorators;
};
typedef struct _cubec_statement_cunion_init_t cubec_statement_cunion_init_t;

/**
 * @brief Try to parse a cunion declaration statement.
 * @param allocator The allocator to use.
 * @param tokens The token list.
 * @param position Current position in token list (updated on success).
 * @param filename The source filename for error reporting.
 * @return A new cubec_statement_cunion_t node, or NULL if current token
 *         is not 'cunion' keyword.
 */
node_t read_statement_cunion(context_t ctx, vec_t tokens, size_t *position,
                             const char *filename);

node_t create_statement_cunion(context_t ctx, location_t loc, const char *name,
                               vec_t fields);

#ifdef __cplusplus
}
#endif
#endif
