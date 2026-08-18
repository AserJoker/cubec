#ifndef _H_CUBEC_CUBEC_STATEMENT_TEST_
#define _H_CUBEC_CUBEC_STATEMENT_TEST_
#include "core/location.h"
#include "core/node.h"
#include "core/string.h"
#include "core/class.h"
#include "core/vec.h"
#include "core/emit_context.h"
#include "engine/vm.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AST node for test block statement.
 *
 * Syntax:
 *   test "<name>" { <body> }
 *
 * Test blocks are top-level only. The name is a string literal (required).
 * The body is a block statement.
 *
 * Examples:
 *   test "basic arithmetic" { }
 *   test "string operations" { }
 */
struct _cubec_statement_test_t;
struct _cubec_statement_test_t {
  struct _node_t super;
  string_t name; /**< Test name (required) */
  node_t body;   /**< Block statement (required) */
};
typedef struct _cubec_statement_test_t *cubec_statement_test_t;

extern class_t g_cubec_statement_test_class;

struct _cubec_statement_test_init_t {
  location_t location;
  node_t parent;
  string_t name;
  node_t body;
};
typedef struct _cubec_statement_test_init_t cubec_statement_test_init_t;

/**
 * @brief Try to parse a test block statement.
 */
node_t read_statement_test(vm_t vm, vec_t tokens, size_t *position,
                           const char *filename);

node_t create_statement_test(vm_t vm, location_t loc, const char *name,
                             node_t body);


void emit_statement_test(emit_context_t ctx, node_t stmt);

#ifdef __cplusplus
}
#endif
#endif
