#ifndef _H_CUBEC_CUBEC_STATEMENT_FOREACH_
#define _H_CUBEC_CUBEC_STATEMENT_FOREACH_
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AST node for foreach iterator loop statement.
 *
 * Syntax:
 *   foreach(<identifier> of <iterator>) <statement>
 *   foreach(var <identifier>[:<type>] of <iterator>) <statement>
 *
 * The 'var' modifier declares a new loop variable; without it the
 * identifier is an existing lvalue.  The optional ':<type>' provides
 * an explicit type annotation.  The 'of' keyword separates the
 * variable from the iterator expression.
 *
 * Examples:
 *   foreach(item of collection) { }
 *   foreach(var item of items) { }
 *   foreach(var item: i32 of items) { }
 */
struct _cubec_statement_foreach_t;
struct _cubec_statement_foreach_t {
  struct _node_t super;
  bool is_var_decl; /**< true = "var name[:type]", false = lvalue */
  node_t variable;  /**< Identifier node for the loop variable */
  node_t var_type;  /**< Optional type annotation (NULL if none) */
  node_t iterator;  /**< Iterator expression */
  node_t body;      /**< Body statement */
};
typedef struct _cubec_statement_foreach_t *cubec_statement_foreach_t;

extern type_t g_cubec_statement_foreach_type;

struct _cubec_statement_foreach_init_t {
  location_t location;
  node_t parent;
  bool is_var_decl;
  node_t variable;
  node_t var_type;
  node_t iterator;
  node_t body;
};
typedef struct _cubec_statement_foreach_init_t cubec_statement_foreach_init_t;

/**
 * @brief Try to parse a foreach statement.
 */
node_t read_statement_foreach(context_t ctx, vec_t tokens, size_t *position,
                              const char *filename);

node_t create_statement_foreach(context_t ctx, location_t loc, bool is_var_decl,
                                node_t variable, node_t var_type,
                                node_t iterator, node_t body);

#ifdef __cplusplus
}
#endif
#endif
