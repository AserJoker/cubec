#ifndef _H_CUBEC_CUBEC_DECLARATION_VARIABLE_
#define _H_CUBEC_CUBEC_DECLARATION_VARIABLE_
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#include "core/writer.h"
#include "cubec/declaration.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _cubec_declaration_variable_t;
struct _cubec_declaration_variable_t {
  struct _cubec_declaration_t super;
  node_t identifier; /**< The variable identifier */
  node_t type;       /**< Optional type annotation (NULL if omitted) */
  node_t expression; /**< The initializer expression (NULL for extern/builtin
                        declarations) */
};
typedef struct _cubec_declaration_variable_t *cubec_declaration_variable_t;

extern type_t g_cubec_declaration_variable_type;

struct _cubec_declaration_variable_init_t {
  location_t location;
  node_t parent;
  node_t identifier;
  node_t type;
  node_t expression;
};
typedef struct _cubec_declaration_variable_init_t
    cubec_declaration_variable_init_t;

/**
 * @brief Try to parse a variable declarator: <identifier> [: <type>] [=
 * <expression>]
 * @param allocator The allocator to use
 * @param tokens The token list
 * @param position Current position in token list (updated on success)
 * @param filename The source filename for error reporting
 * @return A new cubec_declaration_variable_t node, or NULL if current token
 *         is not an identifier.
 */
node_t read_declaration_variable(context_t ctx, vec_t tokens, size_t *position,
                                 const char *filename);

node_t create_declaration_variable(context_t ctx, location_t loc,
                                   node_t identifier, node_t type,
                                   node_t expression);

void write_declaration_variable(writer_t writer, node_t node);

#ifdef __cplusplus
}
#endif
#endif