#ifndef _H_CUBEC_ENGINE_CHECKER_TYPE_UTIL_
#define _H_CUBEC_ENGINE_CHECKER_TYPE_UTIL_
#include "engine/context.h"
#include "core/node.h"
#include "core/strmap.h"
#include "engine/semantic_type.h"
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif

/* identifier helper */
const char *_checker_ident_str(node_t id_node);

/* type predicates */
bool _is_numeric_type(semantic_type_t t);
bool _is_integer_type(semantic_type_t t);
bool _is_bool_type(semantic_type_t t);
bool _is_comparable_type(semantic_type_t t);
bool _is_lvalue(node_t expr);
bool _is_struct_like(semantic_type_t t);
vec_t _get_struct_fields(semantic_type_t t);

/* type utilities */
semantic_type_t _common_type(context_t ctx, semantic_type_t a,
                             semantic_type_t b);

/* struct/union/enum field resolution — unified for global and local */
void _resolve_struct_fields(context_t ctx, semantic_type_t t, vec_t members);
void _resolve_union_fields(context_t ctx, semantic_type_t t, vec_t members);
void _resolve_enum_items(context_t ctx, semantic_type_t t, vec_t items);

/* literal numeric helper */
semantic_type_t _check_literal_numeric(context_t ctx, node_t num_node);

/* generic instantiation helpers */
char *_generic_instance_cache_key(context_t ctx, const char *template_name,
                                   strmap_t type_bindings);
bool _check_constraint(context_t ctx, semantic_type_t type_arg,
                       semantic_type_t constraint, node_t arg_expr);
bool _check_constraint_silent(context_t ctx, semantic_type_t type_arg,
                              semantic_type_t constraint);
vec_t _resolve_generic_type_args(context_t ctx, vec_t arg_exprs, vec_t generic_params);
strmap_t _resolve_generic_type_bindings_pack(context_t ctx, vec_t arg_exprs, vec_t generic_params);
semantic_type_t _instantiate_type(context_t ctx, semantic_type_t template_type,
                                   strmap_t type_bindings, node_t instantiation_expr);
semantic_type_t _instantiate_function(context_t ctx, struct symbol *func_sym,
                                      strmap_t type_bindings, node_t instantiation_expr);

/* Substitute generic params in a type with concrete type bindings */
semantic_type_t _substitute_type(context_t ctx, semantic_type_t type,
                                   strmap_t type_bindings);

/* type unification for generic inference */
strmap_t _infer_type_args_from_call(context_t ctx,
                                  semantic_type_t func_type,
                                  vec_t arg_types,
                                  strmap_t explicit_bindings);

/* Check generic param constraints */
bool _check_generic_param_constraints(context_t ctx, vec_t generic_params,
                                       strmap_t type_bindings, node_t expr);

#ifdef __cplusplus
}
#endif
#endif
