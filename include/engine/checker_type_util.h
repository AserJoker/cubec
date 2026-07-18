#ifndef _H_CUBEC_ENGINE_CHECKER_TYPE_UTIL_
#define _H_CUBEC_ENGINE_CHECKER_TYPE_UTIL_
#include "engine/checker.h"
#include "core/node.h"
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
bool _is_lvalue(node_t expr);
bool _is_struct_like(semantic_type_t t);
vec_t _get_struct_fields(semantic_type_t t);

/* type utilities */
semantic_type_t _common_type(checker_t ctx, semantic_type_t a,
                             semantic_type_t b);

/* literal numeric helper */
semantic_type_t _check_literal_numeric(checker_t ctx, node_t num_node);

/* generic instantiation helpers */
char *_generic_instance_cache_key(checker_t ctx, const char *template_name,
                                   vec_t type_args);
bool _check_constraint(checker_t ctx, semantic_type_t type_arg,
                       semantic_type_t constraint, node_t arg_expr);
vec_t _resolve_generic_type_args(checker_t ctx, vec_t arg_exprs, vec_t generic_params);
semantic_type_t _instantiate_type(checker_t ctx, semantic_type_t template_type,
                                   vec_t type_args, node_t instantiation_expr);
semantic_type_t _instantiate_function(checker_t ctx, struct symbol *func_sym,
                                      vec_t type_args, node_t instantiation_expr);

/* type unification for generic inference */
vec_t _infer_type_args_from_call(checker_t ctx,
                                  semantic_type_t func_type,
                                  vec_t generic_params,
                                  vec_t arg_types,
                                  vec_t explicit_type_args);

/* Check generic param constraints */
bool _check_generic_param_constraints(checker_t ctx, vec_t generic_params,
                                       vec_t type_args, node_t expr);

#ifdef __cplusplus
}
#endif
#endif
