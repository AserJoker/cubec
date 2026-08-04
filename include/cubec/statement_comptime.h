#ifndef _H_CUBEC_CUBEC_STATEMENT_COMPTIME_
#define _H_CUBEC_CUBEC_STATEMENT_COMPTIME_
#include "engine/context.h"
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#include "core/emit_context.h"
#include "core/writer.h"
#ifdef __cplusplus
extern "C" {
#endif

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
 * @brief AST node for comptime foreach: comptime foreach(var item [: type] of iter) { }
 *
 * Compile-time iterator expansion. The iterator must be evaluable at
 * compile time and the loop is expanded at compile time.
 */
struct _cubec_statement_comptime_foreach_t;
struct _cubec_statement_comptime_foreach_t {
  struct _node_t super;
  bool is_var_decl;   /**< True if 'var' keyword was used */
  node_t variable;    /**< Loop variable identifier (required) */
  node_t var_type;    /**< Loop variable type annotation (nullable) */
  node_t iterator;    /**< Iterator expression (required) */
  node_t body;        /**< Block statement (required) */
};
typedef struct _cubec_statement_comptime_foreach_t *cubec_statement_comptime_foreach_t;

extern type_t g_cubec_statement_comptime_foreach_type;

struct _cubec_statement_comptime_foreach_init_t {
  location_t location;
  node_t parent;
  bool is_var_decl;
  node_t variable;
  node_t var_type;
  node_t iterator;
  node_t body;
};
typedef struct _cubec_statement_comptime_foreach_init_t
    cubec_statement_comptime_foreach_init_t;

/**
 * @brief Try to parse a comptime statement: comptime { } | comptime if() | comptime for()
 *
 * Checks for the 'comptime' keyword followed by '{', 'if', or 'for'.
 * Returns NULL if 'comptime' is followed by 'var', 'func', or other modifiers
 * (those are handled by declaration/function parsers).
 */
node_t read_statement_comptime(context_t ctx, vec_t tokens,
                                size_t *position, const char *filename);

node_t create_statement_comptime_if(context_t ctx, location_t loc,
                                    node_t condition, node_t then_branch,
                                    node_t else_branch);
node_t create_statement_comptime_foreach(context_t ctx, location_t loc,
                                         bool is_var_decl, node_t variable,
                                         node_t var_type, node_t iterator,
                                         node_t body);

void write_statement_comptime_if(writer_t writer, node_t node);
void write_statement_comptime_foreach(writer_t writer, node_t node);

void emit_statement_comptime_if(emit_context_t ctx, node_t node);
void emit_statement_comptime_foreach(emit_context_t ctx, node_t node);

#ifdef __cplusplus
}
#endif
#endif
