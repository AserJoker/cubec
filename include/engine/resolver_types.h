#ifndef _H_CUBEC_ENGINE_RESOLVER_TYPES_
#define _H_CUBEC_ENGINE_RESOLVER_TYPES_
#include "engine/context.h"
#include "core/node.h"
#include "engine/semantic_type.h"
#ifdef __cplusplus
extern "C" {
#endif

/* identifier helper */
const char *_resolver_ident_str(node_t id_node);

/* type resolution sub-functions */
semantic_type_t _resolve_type_identifier(context_t ctx, node_t node);
semantic_type_t _resolve_type_pointer(context_t ctx, node_t node);
semantic_type_t _resolve_type_slice(context_t ctx, node_t node);
semantic_type_t _resolve_type_array(context_t ctx, node_t node);
semantic_type_t _resolve_type_qualifier(context_t ctx, node_t node);
semantic_type_t _resolve_type_struct(context_t ctx, node_t node);
semantic_type_t _resolve_type_enum(context_t ctx, node_t node);
semantic_type_t _resolve_type_union(context_t ctx, node_t node);
semantic_type_t _resolve_type_interface(context_t ctx, node_t node);
semantic_type_t _resolve_type_function(context_t ctx, node_t node);
semantic_type_t _resolve_type_namespace_access(context_t ctx, node_t node);
semantic_type_t _resolve_type_typeof(context_t ctx, node_t node);
semantic_type_t _resolve_type_tuple(context_t ctx, node_t node);

#ifdef __cplusplus
}
#endif
#endif
