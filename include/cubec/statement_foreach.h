#ifndef _H_CUBEC_CUBEC_STATEMENT_FOREACH_
#define _H_CUBEC_CUBEC_STATEMENT_FOREACH_
#include "core/allocator.h"
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AST node for foreach iterator loop statement.
 *
 * Syntax:
 *   foreach(const <identifier> : <iterator>) { <body> }
 *   foreach(<identifier> : <iterator>) { <body> }
 *
 * The 'const' modifier is optional. The iterator is an expression
 * that evaluates to an iterable value.
 *
 * Examples:
 *   foreach(const item: collection) { }
 *   foreach(item: items) { }
 */
struct _cubec_statement_foreach_t;
struct _cubec_statement_foreach_t {
  struct _node_t super;
  bool is_const;     /**< Whether the loop variable is const */
  node_t name;       /**< Identifier node for the loop variable */
  node_t iterator;   /**< Iterator expression */
  node_t body;       /**< Block statement (required) */
};
typedef struct _cubec_statement_foreach_t *cubec_statement_foreach_t;

extern type_t g_cubec_statement_foreach_type;

struct _cubec_statement_foreach_init_t {
  location_t location;
  node_t parent;
  bool is_const;
  node_t name;
  node_t iterator;
  node_t body;
};
typedef struct _cubec_statement_foreach_init_t cubec_statement_foreach_init_t;

/**
 * @brief Try to parse a foreach statement.
 */
node_t read_statement_foreach(allocator_t allocator, vec_t tokens,
                               size_t *position, const char *filename);

#ifdef __cplusplus
}
#endif
#endif
