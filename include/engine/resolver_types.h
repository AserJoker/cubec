#ifndef _H_CUBEC_ENGINE_RESOLVER_TYPES_
#define _H_CUBEC_ENGINE_RESOLVER_TYPES_
#include "engine/checker.h"
#include "core/node.h"
#include "engine/semantic_type.h"
#ifdef __cplusplus
extern "C" {
#endif

/* identifier helper */
const char *_resolver_ident_str(node_t id_node);

/* type resolution sub-functions */
semantic_type_t _resolve_type_identifier(checker_t ctx, node_t node);
semantic_type_t _resolve_type_pointer(checker_t ctx, node_t node);
semantic_type_t _resolve_type_slice(checker_t ctx, node_t node);
semantic_type_t _resolve_type_array(checker_t ctx, node_t node);
semantic_type_t _resolve_type_qualifier(checker_t ctx, node_t node);
semantic_type_t _resolve_type_struct(checker_t ctx, node_t node);
semantic_type_t _resolve_type_enum(checker_t ctx, node_t node);
semantic_type_t _resolve_type_union(checker_t ctx, node_t node);
semantic_type_t _resolve_type_interface(checker_t ctx, node_t node);
semantic_type_t _resolve_type_function(checker_t ctx, node_t node);
semantic_type_t _resolve_type_namespace_access(checker_t ctx, node_t node);
semantic_type_t _resolve_type_typeof(checker_t ctx, node_t node);

#ifdef __cplusplus
}
#endif
#endif
