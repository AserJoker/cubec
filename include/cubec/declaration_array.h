#ifndef _H_CUBEC_CUBEC_DECLARATION_ARRAY_
#define _H_CUBEC_CUBEC_DECLARATION_ARRAY_
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#include "cubec/declaration.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _cubec_declaration_array_t;
struct _cubec_declaration_array_t {
  struct _cubec_declaration_t super;
  node_t size; /**< The array size expression */
  node_t type; /**< The underlying type */
};
typedef struct _cubec_declaration_array_t *cubec_declaration_array_t;

extern type_t g_cubec_declaration_array_type;

struct _cubec_declaration_array_init_t {
  location_t location;
  node_t parent;
  node_t size;
  node_t type;
};
typedef struct _cubec_declaration_array_init_t cubec_declaration_array_init_t;

/**
 * @brief Try to parse an array declaration: [ <expr> ] <type>
 * @param allocator The allocator to use
 * @param tokens The token list
 * @param position Current position in token list (updated on success)
 * @param filename The source filename for error reporting
 * @return A new cubec_declaration_array_t node, or NULL if current token
 *         is not '[' followed by non-']' token.
 */
node_t read_declaration_array(context_t ctx, vec_t tokens, size_t *position,
                              const char *filename);

node_t cubec_ast_create_array_type(context_t ctx, location_t loc, node_t size,
                                   node_t base);

#ifdef __cplusplus
}
#endif
#endif
