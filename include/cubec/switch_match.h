#ifndef _H_CUBEC_CUBEC_SWITCH_MATCH_
#define _H_CUBEC_CUBEC_SWITCH_MATCH_
#include "core/allocator.h"
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AST node for a switch case/else match arm.
 *
 * Syntax:
 *   case(value1, value2, ...) -> { <body> }
 *   else -> { <body> }
 *
 * The 'case' form has one or more match values separated by commas.
 * The 'else' form has no values (is_else = true).
 *
 * Examples:
 *   case(1) -> { }
 *   case(2, 3) -> { }
 *   else -> { }
 */
struct _cubec_switch_match_t;
struct _cubec_switch_match_t {
  struct _node_t super;
  bool is_else;     /**< Whether this is the else (default) arm */
  vec_t values;     /**< Match value expressions (auto_dispose, empty for else) */
  node_t body;      /**< Block statement (required) */
};
typedef struct _cubec_switch_match_t *cubec_switch_match_t;

extern type_t g_cubec_switch_match_type;

struct _cubec_switch_match_init_t {
  location_t location;
  node_t parent;
  bool is_else;
  vec_t values;
  node_t body;
};
typedef struct _cubec_switch_match_init_t cubec_switch_match_init_t;

/**
 * @brief Try to parse a switch match arm (case or else).
 */
node_t read_switch_match(allocator_t allocator, vec_t tokens,
                          size_t *position, const char *filename);

#ifdef __cplusplus
}
#endif
#endif
