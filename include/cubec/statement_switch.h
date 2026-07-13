#ifndef _H_CUBEC_CUBEC_STATEMENT_SWITCH_
#define _H_CUBEC_CUBEC_STATEMENT_SWITCH_
#include "core/allocator.h"
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AST node for switch statement.
 *
 * Syntax:
 *   switch(value) {
 *     case(v1, v2) -> { <body> }
 *     else -> { <body> }
 *   }
 *
 * The switch expression is compared against each case arm's values.
 * The else arm is the default fallback.
 *
 * Examples:
 *   switch(x) { case(1) -> { } case(2, 3) -> { } else -> { } }
 */
struct _cubec_statement_switch_t;
struct _cubec_statement_switch_t {
  struct _node_t super;
  node_t condition;  /**< Switch expression (required) */
  vec_t matches;     /**< Switch match arms (auto_dispose, switch_match nodes) */
};
typedef struct _cubec_statement_switch_t *cubec_statement_switch_t;

extern type_t g_cubec_statement_switch_type;

struct _cubec_statement_switch_init_t {
  location_t location;
  node_t parent;
  node_t condition;
  vec_t matches;
};
typedef struct _cubec_statement_switch_init_t cubec_statement_switch_init_t;

/**
 * @brief Try to parse a switch statement.
 */
node_t read_statement_switch(allocator_t allocator, vec_t tokens,
                              size_t *position, const char *filename);

#ifdef __cplusplus
}
#endif
#endif
