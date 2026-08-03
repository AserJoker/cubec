#ifndef _H_CUBEC_CUBEC_EXPRESSION_INITIALIZE_LIST_
#define _H_CUBEC_CUBEC_EXPRESSION_INITIALIZE_LIST_
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#include "core/writer.h"
#include "cubec/expression.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _cubec_expression_initialize_list_t;
struct _cubec_expression_initialize_list_t {
  struct _cubec_expression_t super;
  node_t type;   /**< Optional type expression (NULL for anonymous .{...}) */
  vec_t items;   /**< vec_t of items — all initialize_field or all expression */
  bool is_field; /**< true = items are initialize_field; false = positional
                    expressions */
};
typedef struct _cubec_expression_initialize_list_t
    *cubec_expression_initialize_list_t;

extern type_t g_cubec_expression_initialize_list_type;

struct _cubec_expression_initialize_list_init_t {
  location_t location;
  node_t parent;
  node_t type;
  vec_t items;
  bool is_field;
};
typedef struct _cubec_expression_initialize_list_init_t
    cubec_expression_initialize_list_init_t;

/**
 * @brief Try to parse an initialize list expression: \c .<type>{<items>} or \c
 * .{<items>}
 *
 * Called from \c read_atom as a primary expression. Checks for \c '.' at the
 * current position, then looks ahead for \c '{' (anonymous) or identifier
 * followed by \c '{' (typed).
 *
 * Items are comma-separated and must be either all initialize_field
 * (\c .name = value) or all positional expressions — mixing is an error.
 *
 * @return A new cubec_expression_initialize_list_t node, or NULL if the
 *         current token is not \c '.' followed by \c '{' or identifier+\c {.
 */
node_t read_expression_initialize_list(context_t ctx, vec_t tokens,
                                       size_t *position, const char *filename);

node_t create_expression_initialize_list(context_t ctx, location_t loc,
                                         node_t type, vec_t items,
                                         bool is_field);

void write_expression_initialize_list(writer_t writer, node_t node);

#ifdef __cplusplus
}
#endif
#endif
