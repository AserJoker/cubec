#ifndef _H_CUBEC_CUBEC_EXPRESSION_DEREF_
#define _H_CUBEC_CUBEC_EXPRESSION_DEREF_
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

/**
 * @brief Postfix dereference expression: <host>.*
 *        Dereferences a pointer value.
 */
struct _cubec_expression_deref_t;
struct _cubec_expression_deref_t {
  struct _cubec_expression_t super;
  node_t host; /**< The pointer operand being dereferenced */
};
typedef struct _cubec_expression_deref_t *cubec_expression_deref_t;

extern type_t g_cubec_expression_deref_type;

struct _cubec_expression_deref_init_t {
  location_t location;
  node_t parent;
  node_t host;
};
typedef struct _cubec_expression_deref_init_t cubec_expression_deref_init_t;

/**
 * @brief Try to parse a postfix dereference expression: <host>.*
 * @param host The already-parsed left operand (the pointer to dereference)
 * @return A new cubec_expression_deref_t node, or NULL if the current
 *         token is not '.' followed by '*'.
 */
node_t read_expression_deref(context_t ctx, vec_t tokens, size_t *position,
                             const char *filename, node_t host);

node_t create_expression_deref(context_t ctx, location_t loc, node_t host);

void write_expression_deref(writer_t writer, node_t node);

#ifdef __cplusplus
}
#endif
#endif
