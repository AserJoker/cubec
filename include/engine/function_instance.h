#ifndef _H_CUBEC_ENGINE_FUNCTION_INSTANCE_
#define _H_CUBEC_ENGINE_FUNCTION_INSTANCE_

#include "core/allocator.h"
#include "core/node.h"
#include "core/vec.h"
#include "engine/stype.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Function instance — a concrete instantiation of a function_t template.
 *
 * arguments and return_type are borrowing (point into context->types).
 * body is borrowing (points into the AST).
 * captures is owned (vec of captured variable references).
 */
struct _function_instance_t {
  allocator_t allocator;
  vec_t arguments;     /* vec of stype_t: concrete parameter types (borrowing) */
  stype_t return_type; /* concrete return type (borrowing) */
  node_t body;         /* AST body node (borrowing, NULL for extern/builtin) */
  vec_t captures;      /* captured variable references (owned, nullable) */
};

typedef struct _function_instance_t *function_instance_t;

/**
 * @brief Create a function_instance_t.
 * @param allocator    Allocator for this object
 * @param arguments    vec of stype_t parameter types (borrowing, may be NULL)
 * @param return_type  Return type (borrowing)
 * @param body         AST body node (borrowing, may be NULL)
 * @param captures     vec of captured variable references (owned, may be NULL)
 */
function_instance_t function_instance_create(allocator_t allocator,
                                             vec_t arguments,
                                             stype_t return_type,
                                             node_t body,
                                             vec_t captures);

/** @brief Dispose a function_instance_t and its owned sub-objects. */
void function_instance_dispose(function_instance_t inst);

#ifdef __cplusplus
}
#endif
#endif /* _H_CUBEC_ENGINE_FUNCTION_INSTANCE_ */
