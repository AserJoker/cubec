#ifndef _H_CUBEC_CUBEC_STATEMENT_DECLARATION_
#define _H_CUBEC_CUBEC_STATEMENT_DECLARATION_
#include "core/allocator.h"
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _cubec_statement_declaration_t;
struct _cubec_statement_declaration_t {
  struct _node_t super;
  bool is_export;     /**< Whether this declaration is exported */
  vec_t declarators;  /**< Vector of cubec_declaration_variable_t */
};
typedef struct _cubec_statement_declaration_t *cubec_statement_declaration_t;

extern type_t g_cubec_statement_declaration_type;

struct _cubec_statement_declaration_init_t {
  location_t location;
  node_t parent;
  bool is_export;
  vec_t declarators;
};
typedef struct _cubec_statement_declaration_init_t cubec_statement_declaration_init_t;

/**
 * @brief Try to parse a declaration statement: var <declaration_variable> [, <declaration_variable>]* ;
 * @param allocator The allocator to use
 * @param tokens The token list
 * @param position Current position in token list (updated on success)
 * @param filename The source filename for error reporting
 * @return A new cubec_statement_declaration_t node, or NULL if current token
 *         is not 'var'.
 */
node_t read_statement_declaration(allocator_t allocator, vec_t tokens,
                                  size_t *position, const char *filename);

#ifdef __cplusplus
}
#endif
#endif