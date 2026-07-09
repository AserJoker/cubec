#ifndef _H_CUBEC_CUBEC_EXPRESSION_
#define _H_CUBEC_CUBEC_EXPRESSION_
#include "core/allocator.h"
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#include "cubec/node.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _cubec_expression_t;
struct _cubec_expression_t {
  struct _node_t super;
};
typedef struct _cubec_expression_t *cubec_expression_t;

extern type_t g_cubec_expression_type;

struct _cubec_expression_init_t {
  location_t location;
  node_t parent;
  cubec_node_kind_t kind;
};
typedef struct _cubec_expression_init_t cubec_expression_init_t;

node_t read_atom(allocator_t allocator, vec_t tokens, size_t *position,
                 const char *filename);

node_t read_value(allocator_t allocator, vec_t tokens, size_t *position,
                  const char *filename);

/** @brief Parse a full type expression, trying ternary type first and
 *  falling back to primary (non-ternary) type expression */
node_t read_expression_type(allocator_t allocator, vec_t tokens,
                            size_t *position, const char *filename);

/** @brief Parse a primary (non-ternary) type expression: identifier with
 *  optional member access, generic instantiation, pointer/slice/array
 *  declaration, and grouping.
 *  @note Internal helper: should only be called by read_expression_type
 *        and read_expression_type_ternary. External callers should use
 *        read_expression_type. */
node_t read_type_expression_primary(allocator_t allocator, vec_t tokens,
                                    size_t *position, const char *filename);

node_t read_expression(allocator_t allocator, vec_t tokens, size_t *position,
                       const char *filename);

#ifdef __cplusplus
}
#endif
#endif
