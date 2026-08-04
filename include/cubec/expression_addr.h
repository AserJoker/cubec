#ifndef _H_CUBEC_CUBEC_EXPRESSION_ADDR_
#define _H_CUBEC_CUBEC_EXPRESSION_ADDR_
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#include "core/emit_context.h"
#include "core/writer.h"
#include "cubec/expression.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Postfix address-of expression: <host>.&
 *        Takes the address of the host expression, returning a *T pointer.
 */
struct _cubec_expression_addr_t;
struct _cubec_expression_addr_t {
  struct _cubec_expression_t super;
  node_t host; /**< The operand whose address is taken */
};
typedef struct _cubec_expression_addr_t *cubec_expression_addr_t;

extern type_t g_cubec_expression_addr_type;

struct _cubec_expression_addr_init_t {
  location_t location;
  node_t parent;
  node_t host;
};
typedef struct _cubec_expression_addr_init_t cubec_expression_addr_init_t;

/**
 * @brief Try to parse a postfix address-of expression: <host>.&
 * @param host The already-parsed left operand (the value before the operator)
 * @return A new cubec_expression_addr_t node, or NULL if the current
 *         token is not '.' followed by '&'.
 */
node_t read_expression_addr(context_t ctx, vec_t tokens, size_t *position,
                            const char *filename, node_t host);

node_t create_expression_addr(context_t ctx, location_t loc, node_t host);

void write_expression_addr(writer_t writer, node_t node);

void emit_expression_addr(emit_context_t ctx, node_t node);

#ifdef __cplusplus
}
#endif
#endif
