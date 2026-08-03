#ifndef _H_CUBEC_CUBEC_DECLARATION_POINTER_
#define _H_CUBEC_CUBEC_DECLARATION_POINTER_
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

struct _cubec_declaration_pointer_t;
struct _cubec_declaration_pointer_t {
  struct _cubec_declaration_t super;
  node_t type;      /**< The underlying type */
  bool is_const;    /**< Whether const qualifier is present */
  bool is_volatile; /**< Whether volatile qualifier is present */
};
typedef struct _cubec_declaration_pointer_t *cubec_declaration_pointer_t;

extern type_t g_cubec_declaration_pointer_type;

struct _cubec_declaration_pointer_init_t {
  location_t location;
  node_t parent;
  node_t type;
  bool is_const;
  bool is_volatile;
};
typedef struct _cubec_declaration_pointer_init_t
    cubec_declaration_pointer_init_t;

/**
 * @brief Try to parse a pointer declaration: * [const] [volatile] <type>
 * @param allocator The allocator to use
 * @param tokens The token list
 * @param position Current position in token list (updated on success)
 * @param filename The source filename for error reporting
 * @return A new cubec_declaration_pointer_t node, or NULL if current token
 *         is not '*'.
 */
node_t read_declaration_pointer(context_t ctx, vec_t tokens, size_t *position,
                                const char *filename);

node_t create_declaration_pointer(context_t ctx, location_t loc, node_t base,
                                  bool is_const, bool is_volatile);

void write_declaration_pointer(writer_t writer, node_t node);

#ifdef __cplusplus
}
#endif
#endif