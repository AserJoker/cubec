#ifndef _H_CUBEC_CUBEC_STATEMENT_COMPTIME_
#define _H_CUBEC_CUBEC_STATEMENT_COMPTIME_
#include "core/allocator.h"
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AST node for comptime block: comptime { <body> }
 *
 * The body is executed at compile time. The block introduces a new scope.
 */
struct _cubec_statement_comptime_block_t;
struct _cubec_statement_comptime_block_t {
  struct _node_t super;
  node_t body;   /**< Block statement (required) */
};
typedef struct _cubec_statement_comptime_block_t *cubec_statement_comptime_block_t;

extern type_t g_cubec_statement_comptime_block_type;

struct _cubec_statement_comptime_block_init_t {
  location_t location;
  node_t parent;
  node_t body;
};
typedef struct _cubec_statement_comptime_block_init_t
    cubec_statement_comptime_block_init_t;

/**
 * @brief AST node for comptime if: comptime if(condition) { } [else { }]
 *
 * Compile-time conditional branch. The condition must be evaluable at
 * compile time. Non-taken branches are excluded from code generation.
 */
struct _cubec_statement_comptime_if_t;
struct _cubec_statement_comptime_if_t {
  struct _node_t super;
  node_t condition;    /**< Compile-time condition (required) */
  node_t then_branch;  /**< Then block (required) */
  node_t else_branch;  /**< Else block or nested comptime if (nullable) */
};
typedef struct _cubec_statement_comptime_if_t *cubec_statement_comptime_if_t;

extern type_t g_cubec_statement_comptime_if_type;

struct _cubec_statement_comptime_if_init_t {
  location_t location;
  node_t parent;
  node_t condition;
  node_t then_branch;
  node_t else_branch;
};
typedef struct _cubec_statement_comptime_if_init_t
    cubec_statement_comptime_if_init_t;

/**
 * @brief AST node for comptime for: comptime for(init; cond; incr) { }
 *
 * Compile-time loop unrolling. The loop is unrolled at compile time.
 */
struct _cubec_statement_comptime_for_t;
struct _cubec_statement_comptime_for_t {
  struct _node_t super;
  node_t init;       /**< Init statement/expression (nullable) */
  node_t condition;  /**< Condition expression (nullable) */
  node_t increment;  /**< Increment expression (nullable) */
  node_t body;       /**< Block statement (required) */
};
typedef struct _cubec_statement_comptime_for_t *cubec_statement_comptime_for_t;

extern type_t g_cubec_statement_comptime_for_type;

struct _cubec_statement_comptime_for_init_t {
  location_t location;
  node_t parent;
  node_t init;
  node_t condition;
  node_t increment;
  node_t body;
};
typedef struct _cubec_statement_comptime_for_init_t
    cubec_statement_comptime_for_init_t;

/**
 * @brief Try to parse a comptime statement: comptime { } | comptime if() | comptime for()
 *
 * Checks for the 'comptime' keyword followed by '{', 'if', or 'for'.
 * Returns NULL if 'comptime' is followed by 'var', 'func', or other modifiers
 * (those are handled by declaration/function parsers).
 */
node_t read_statement_comptime(allocator_t allocator, vec_t tokens,
                                size_t *position, const char *filename);

#ifdef __cplusplus
}
#endif
#endif
