#ifndef _H_CUBEC_CUBEC_STATEMENT_DEFER_
#define _H_CUBEC_CUBEC_STATEMENT_DEFER_
#include "core/allocator.h"
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AST node for defer statement.
 *
 * Syntax:
 *   defer <expression>;
 *   defer { <body> }
 *
 * The defer statement schedules execution at the end of the current scope.
 * Two forms: expression (followed by semicolon) or block.
 *
 * Examples:
 *   defer cleanup();
 *   defer { file.close(); }
 */
struct _cubec_statement_defer_t;
struct _cubec_statement_defer_t {
  struct _node_t super;
  node_t body;       /**< Expression statement or block (required) */
};
typedef struct _cubec_statement_defer_t *cubec_statement_defer_t;

extern type_t g_cubec_statement_defer_type;

struct _cubec_statement_defer_init_t {
  location_t location;
  node_t parent;
  node_t body;
};
typedef struct _cubec_statement_defer_init_t cubec_statement_defer_init_t;

/**
 * @brief Try to parse a defer statement.
 */
node_t read_statement_defer(allocator_t allocator, vec_t tokens,
                             size_t *position, const char *filename);

#ifdef __cplusplus
}
#endif
#endif
