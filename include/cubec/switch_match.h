#ifndef _H_CUBEC_CUBEC_SWITCH_MATCH_
#define _H_CUBEC_CUBEC_SWITCH_MATCH_
#include "core/emit_context.h"
#include "core/location.h"
#include "core/node.h"
#include "core/class.h"
#include "core/vec.h"
#include "engine/vm.h"
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
  bool is_else; /**< Whether this is the else (default) arm */
  vec_t values; /**< Match value expressions (auto_dispose, empty for else) */
  node_t body;  /**< Block statement (required) */
};
typedef struct _cubec_switch_match_t *cubec_switch_match_t;

extern class_t g_cubec_switch_match_class;

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
node_t read_switch_match(vm_t vm, vec_t tokens, size_t *position,
                         const char *filename);

node_t create_switch_match(vm_t vm, location_t loc, bool is_else,
                           vec_t values, node_t body);


void emit_switch_match(emit_context_t ctx, node_t node);

#ifdef __cplusplus
}
#endif
#endif
