#ifndef _H_CUBEC_CUBEC_EXPRESSION_
#define _H_CUBEC_CUBEC_EXPRESSION_
#include "core/location.h"
#include "core/node.h"
#include "core/class.h"
#include "core/vec.h"
#include "core/emit_context.h"
#include "cubec/node.h"
#include "engine/context.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _cubec_expression_t;
struct _cubec_expression_t {
  struct _node_t super;
};
typedef struct _cubec_expression_t *cubec_expression_t;

extern class_t g_cubec_expression_class;

struct _cubec_expression_init_t {
  location_t location;
  node_t parent;
  cubec_node_kind_t kind;
};
typedef struct _cubec_expression_init_t cubec_expression_init_t;

node_t read_atom(context_t ctx, vec_t tokens, size_t *position,
                 const char *filename);

node_t read_value(context_t ctx, vec_t tokens, size_t *position,
                  const char *filename);

/** @brief Parse a type expression. Now identical to read_expression —
 *  type and value expressions share a unified parsing path.
 *  Composite types (pointer/slice/array/qualifier/function type) greedily
 *  consume their inner type, including ternary: const a ? b : c →
 * const(ternary). Use grouping to prevent greedy consumption: (const a) ? b :
 * c. */
node_t read_expression_type(context_t ctx, vec_t tokens, size_t *position,
                            const char *filename);

/** @brief Parse a primary (non-ternary, non-binary) type expression:
 *  identifier with optional namespace access (::), subscript/generic
 *  instantiation (unified [...] syntax), pointer/slice/array declaration,
 *  const/volatile qualifier, typeof/sizeof/alignof, function type, and
 *  grouping.
 *  @note Internal helper for read_atom and type_constraint parsing.
 *        External callers should use read_expression_type. */
node_t read_type_expression_primary(context_t ctx, vec_t tokens,
                                    size_t *position, const char *filename);

/** @brief Parse a base expression (ternary and below — no comma/assignment).
 *  Used as the inner parser for rvalue in assignments and in type contexts
 *  where assignment/comma are not meaningful. */
node_t read_expression_base(context_t ctx, vec_t tokens, size_t *position,
                            const char *filename);

/** @brief Parse a full expression: comma → assignment → ternary → binary →
 *  unary → value. This is the top-level entry point for expression parsing. */
node_t read_expression(context_t ctx, vec_t tokens, size_t *position,
                       const char *filename);


void emit_expression(emit_context_t ctx, node_t expr);

#ifdef __cplusplus
}
#endif
#endif
