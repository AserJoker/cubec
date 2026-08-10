#ifndef _H_CUBEC_CUBEC_DECLARATION_SLICE_
#define _H_CUBEC_CUBEC_DECLARATION_SLICE_
#include "core/location.h"
#include "core/node.h"
#include "core/class.h"
#include "core/vec.h"
#include "cubec/declaration.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _cubec_declaration_slice_t;
struct _cubec_declaration_slice_t {
  struct _cubec_declaration_t super;
  node_t type;      /**< The underlying type */
  bool is_const;    /**< Whether const qualifier is present */
  bool is_volatile; /**< Whether volatile qualifier is present */
};
typedef struct _cubec_declaration_slice_t *cubec_declaration_slice_t;

extern class_t g_cubec_declaration_slice_class;

struct _cubec_declaration_slice_init_t {
  location_t location;
  node_t parent;
  node_t type;
  bool is_const;
  bool is_volatile;
};
typedef struct _cubec_declaration_slice_init_t cubec_declaration_slice_init_t;

/**
 * @brief Try to parse a slice declaration: [] [const] [volatile] <type>
 * @param allocator The allocator to use
 * @param tokens The token list
 * @param position Current position in token list (updated on success)
 * @param filename The source filename for error reporting
 * @return A new cubec_declaration_slice_t node, or NULL if current token
 *         is not '[' followed by ']'.
 */
node_t read_declaration_slice(context_t ctx, vec_t tokens, size_t *position,
                              const char *filename);

node_t create_declaration_slice(context_t ctx, location_t loc, node_t base,
                                bool is_const, bool is_volatile);

void emit_declaration_slice(emit_context_t ctx, node_t node);

#ifdef __cplusplus
}
#endif
#endif