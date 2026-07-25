#ifndef _H_CUBEC_ENGINE_CHECKER_FUNC_UTIL_
#define _H_CUBEC_ENGINE_CHECKER_FUNC_UTIL_
#include "engine/context.h"
#include "engine/symbol.h"
#include "core/node.h"
#include "core/vec.h"
#include "cubec/statement_function.h"
#include "cubec/expression_function.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Unified function check info — abstracts over statement_function and
 *        expression_function AST differences.
 *
 * All checker helper functions operate on this struct instead of directly
 * on the two different AST node types, eliminating code duplication.
 */
typedef struct func_check_info {
  vec_t arguments;       /* function argument vec (cubec_function_argument_t) */
  node_t return_type;    /* return type expression (NULL = void) */
  node_t body;           /* function body (NULL for extern/interface) */
  vec_t captures;        /* capture list (NULL for non-capturing) */
  vec_t generic_params;  /* generic params (NULL for non-generic) */
  bool is_c_variadic;    /* C-style variadic */
  node_t name;           /* identifier node (NULL for anonymous) */
  location_t location;   /* declaration location */
  bool is_comptime;      /* comptime modifier */
  node_t ast_node;       /* original AST node (for comptime dispatch) */
  /* statement_function specific (not used by expression_function) */
  bool is_export;
  bool is_inline;
  bool is_extern;
  bool is_builtin;
  vec_t decorators;
} func_check_info_t;

/* Fill func_check_info from a statement_function node */
void func_check_info_from_statement(func_check_info_t *info,
                                     cubec_statement_function_t node);

/* Fill func_check_info from an expression_function node */
void func_check_info_from_expression(func_check_info_t *info,
                                      cubec_expression_function_t node);

/* Resolve parameter types from func_check_info */
vec_t _resolve_func_param_types(context_t ctx, const func_check_info_t *info);

/* Register parameter symbols in current scope from func_check_info + resolved types */
void _register_func_params_from_info(context_t ctx,
                                      const func_check_info_t *info,
                                      vec_t param_types);

/* Register captures (with TDZ check) in the function scope */
void _register_func_captures(context_t ctx, const func_check_info_t *info,
                              scope_t enclosing_scope);

/* Check function body and return exhaustiveness.
 * scope_root: the parent scope for the new SCOPE_FUNCTION
 *   - global functions: pass saved (= ctx->current_scope before call)
 *   - local functions: pass saved (= enclosing scope)
 *   - global methods: pass ctx->global_scope
 *   - local methods: pass saved (= enclosing scope) */
void _check_func_body_and_returns(context_t ctx,
                                    const func_check_info_t *info,
                                    semantic_type_t return_type,
                                    vec_t param_types,
                                    scope_t scope_root);

/* Unified generic params registration (merges collect.c and evaluate.c versions) */
void context_register_generic_params(context_t ctx, vec_t generic_params);

/**
 * @brief Context controlling how _process_function handles a function declaration.
 *
 * Captures the differences between global, method, local, and expression functions.
 */
typedef struct func_context {
  scope_t symbol_scope;       /* where to bind the symbol (NULL = anonymous) */
  bool defer_body;            /* true = skip body checking here (done in Pass 3) */
  bool is_method;             /* true = push to host_type->instance_methods */
  semantic_type_t host_type;  /* host type for methods */
  bool use_child_scope;       /* true = generic params get child SCOPE_BLOCK (methods) */
  struct symbol *pre_existing_sym; /* pre-collected symbol (global funcs), NULL = create new */
  enum symbol_state symbol_state;  /* SYMBOL_EVALUATED or SYMBOL_NAME_KNOWN */
} func_context_t;

/**
 * @brief Unified function processing: register generics, resolve types,
 *        create function type, bind symbol, check body.
 *
 * Returns the created function type. Callers handle post-processing
 * (builtin validation, comptime binding, etc.) themselves.
 */
semantic_type_t _process_function(context_t ctx, func_check_info_t *info,
                                   func_context_t *fctx);

#ifdef __cplusplus
}
#endif
#endif
