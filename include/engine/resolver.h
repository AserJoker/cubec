#ifndef _H_CUBEC_ENGINE_RESOLVER_
#define _H_CUBEC_ENGINE_RESOLVER_
#include "core/node.h"
#include "engine/semantic_type.h"
#include "engine/scope.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Forward declaration of checker to avoid circular include.
 */
struct context;
typedef struct context *context_t;

/**
 * @brief Resolve an AST type expression node to a semantic_type_t.
 *        Handles:
 *        - literal_identifier: lookup in type_name_table / scope
 *        - declaration_pointer: create pointer type
 *        - declaration_slice: create slice type
 *        - declaration_array: create array type
 *        - expression_qualifier: create const/volatile qualifier
 *        - expression_struct: create anonymous struct type
 *        - expression_union: create anonymous union type
 *        - expression_enum: create anonymous enum type
 *        - expression_interface: create interface type
 *        - expression_callable: create function type
 *        - expression_namespace_access: resolve left::right
 *        - expression_typeof: typeof type
 *
 * @param ctx  The checker context.
 * @param node The AST type expression node.
 * @return The resolved semantic_type_t, or ctx->error_type on failure.
 */
semantic_type_t resolver_resolve_type(context_t ctx, node_t node);

#ifdef __cplusplus
}
#endif
#endif
