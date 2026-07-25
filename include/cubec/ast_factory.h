#ifndef _H_CUBEC_AST_FACTORY_
#define _H_CUBEC_AST_FACTORY_
#include "core/allocator.h"
#include "core/location.h"
#include "core/node.h"
#include "core/vec.h"
#include "cubec/literal_numeric.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif

/* ===== Literals ===== */

node_t cubec_ast_create_identifier(context_t ctx, location_t loc,
                                   const char *name);
node_t cubec_ast_create_numeric(context_t ctx, location_t loc,
                                const char *value,
                                cubec_literal_numeric_kind_t kind,
                                cubec_literal_numeric_type_t ntype);
node_t cubec_ast_create_string(context_t ctx, location_t loc,
                               const char *value);
node_t cubec_ast_create_char(context_t ctx, location_t loc, char value);
node_t cubec_ast_create_undefined(context_t ctx, location_t loc);

/* ===== Expressions ===== */

node_t cubec_ast_create_binary(context_t ctx, location_t loc,
                               const char *op, node_t left, node_t right);
node_t cubec_ast_create_assignment(context_t ctx, location_t loc,
                                   const char *op, node_t lvalue,
                                   node_t rvalue);
node_t cubec_ast_create_call(context_t ctx, location_t loc,
                             node_t callee, vec_t args);
node_t cubec_ast_create_member(context_t ctx, location_t loc,
                               node_t host, const char *field);
node_t cubec_ast_create_namespace_access(context_t ctx, location_t loc,
                                         node_t host, const char *field);
node_t cubec_ast_create_deref(context_t ctx, location_t loc, node_t host);
node_t cubec_ast_create_addr(context_t ctx, location_t loc, node_t host);
node_t cubec_ast_create_try(context_t ctx, location_t loc, node_t host);
node_t cubec_ast_create_assert(context_t ctx, location_t loc, node_t host);
node_t cubec_ast_create_ternary(context_t ctx, location_t loc,
                                node_t cond, node_t then_branch,
                                node_t else_branch);
node_t cubec_ast_create_group(context_t ctx, location_t loc,
                              node_t inner);
node_t cubec_ast_create_typeof(context_t ctx, location_t loc,
                               node_t expr);
node_t cubec_ast_create_sizeof(context_t ctx, location_t loc,
                               node_t expr);
node_t cubec_ast_create_alignof(context_t ctx, location_t loc,
                                node_t expr);
node_t cubec_ast_create_slice_expr(context_t ctx, location_t loc,
                                   node_t host, node_t start,
                                   node_t length);
node_t cubec_ast_create_spread(context_t ctx, location_t loc,
                               node_t value);
node_t cubec_ast_create_comma(context_t ctx, location_t loc,
                              node_t left, node_t right);
node_t cubec_ast_create_generic_instantiation(context_t ctx,
                                              location_t loc,
                                              node_t callee, vec_t args);
node_t cubec_ast_create_type_qualifier(context_t ctx, location_t loc,
                                       node_t base, bool is_const,
                                       bool is_volatile);
node_t cubec_ast_create_initialize_list(context_t ctx, location_t loc,
                                        node_t type, vec_t items,
                                        bool is_field);
node_t cubec_ast_create_initialize_field(context_t ctx, location_t loc,
                                         const char *name, node_t value);
node_t cubec_ast_create_function_expr(context_t ctx, location_t loc,
                                      node_t name, vec_t captures,
                                      vec_t generic_params, vec_t args,
                                      node_t return_type, node_t body,
                                      bool is_c_variadic);

/* ===== Statements ===== */

node_t cubec_ast_create_program(context_t ctx, location_t loc,
                                vec_t statements);
node_t cubec_ast_create_block(context_t ctx, location_t loc,
                              vec_t statements);
node_t cubec_ast_create_struct_stmt(context_t ctx, location_t loc,
                                    const char *name, vec_t members,
                                    bool is_export, vec_t implements);
node_t cubec_ast_create_enum_stmt(context_t ctx, location_t loc,
                                  const char *name, vec_t items,
                                  bool is_export);
node_t cubec_ast_create_union_stmt(context_t ctx, location_t loc,
                                   const char *name, vec_t members,
                                   bool is_export, vec_t implements);
node_t cubec_ast_create_cunion_stmt(context_t ctx, location_t loc,
                                    const char *name, vec_t fields);
node_t cubec_ast_create_func_stmt(context_t ctx, location_t loc,
                                  const char *name, vec_t args,
                                  node_t return_type, node_t body,
                                  bool is_export, bool is_inline,
                                  bool is_extern, bool is_builtin,
                                  bool is_comptime, bool is_c_variadic);
node_t cubec_ast_create_var_decl_stmt(context_t ctx, location_t loc,
                                      const char *name, node_t type,
                                      node_t expr, bool is_export,
                                      bool is_extern, bool is_builtin,
                                      bool is_comptime, bool is_using);
node_t cubec_ast_create_type_alias(context_t ctx, location_t loc,
                                   const char *name, node_t type_value,
                                   bool is_export, bool is_builtin);
node_t cubec_ast_create_iface_stmt(context_t ctx, location_t loc,
                                   const char *name, vec_t members,
                                   bool is_export);
node_t cubec_ast_create_if_stmt(context_t ctx, location_t loc,
                                node_t cond, node_t then_branch,
                                node_t else_branch);
node_t cubec_ast_create_while_stmt(context_t ctx, location_t loc,
                                   node_t cond, node_t body);
node_t cubec_ast_create_do_while_stmt(context_t ctx, location_t loc,
                                      node_t body, node_t cond);
node_t cubec_ast_create_for_stmt(context_t ctx, location_t loc,
                                 node_t init, node_t cond, node_t incr,
                                 node_t body);
node_t cubec_ast_create_foreach_stmt(context_t ctx, location_t loc,
                                     bool is_var_decl, node_t variable,
                                     node_t var_type, node_t iterator,
                                     node_t body);
node_t cubec_ast_create_return_stmt(context_t ctx, location_t loc,
                                    node_t expr);
node_t cubec_ast_create_expr_stmt(context_t ctx, location_t loc,
                                  node_t expr);
node_t cubec_ast_create_break_stmt(context_t ctx, location_t loc);
node_t cubec_ast_create_continue_stmt(context_t ctx, location_t loc);
node_t cubec_ast_create_empty_stmt(context_t ctx, location_t loc);
node_t cubec_ast_create_error(context_t ctx, location_t loc);
node_t cubec_ast_create_error_stmt(context_t ctx, location_t loc);
node_t cubec_ast_create_defer_stmt(context_t ctx, location_t loc,
                                   vec_t captures, node_t body);
node_t cubec_ast_create_switch_stmt(context_t ctx, location_t loc,
                                    node_t cond, vec_t matches);
node_t cubec_ast_create_import_stmt(context_t ctx, location_t loc,
                                    const char *module_name,
                                    const char *alias, const char *path);
node_t cubec_ast_create_test_stmt(context_t ctx, location_t loc,
                                  const char *name, node_t body);

/* ===== Sub-element nodes ===== */

node_t cubec_ast_create_struct_field(context_t ctx, location_t loc,
                                     const char *name, node_t type,
                                     bool is_pub);
node_t cubec_ast_create_enum_item(context_t ctx, location_t loc,
                                  const char *name, node_t type,
                                  node_t value);
node_t cubec_ast_create_union_field(context_t ctx, location_t loc,
                                    const char *name, node_t type);
node_t cubec_ast_create_func_arg(context_t ctx, location_t loc,
                                 const char *name, node_t type);
node_t cubec_ast_create_iface_method(context_t ctx, location_t loc,
                                     const char *name, vec_t args,
                                     node_t return_type);
node_t cubec_ast_create_switch_match(context_t ctx, location_t loc,
                                     bool is_else, vec_t values,
                                     node_t body);
node_t cubec_ast_create_decorator(context_t ctx, location_t loc,
                                  node_t expr);
node_t cubec_ast_create_generic_param(context_t ctx, location_t loc,
                                      const char *name, vec_t constraints,
                                      node_t value_type, bool is_rest);
node_t cubec_ast_create_func_capture(context_t ctx, location_t loc,
                                     const char *name);

/* ===== Type expression nodes ===== */

node_t cubec_ast_create_pointer_type(context_t ctx, location_t loc,
                                     node_t base, bool is_const,
                                     bool is_volatile);
node_t cubec_ast_create_slice_type(context_t ctx, location_t loc,
                                   node_t base, bool is_const,
                                   bool is_volatile);
node_t cubec_ast_create_array_type(context_t ctx, location_t loc,
                                   node_t size, node_t base);
node_t cubec_ast_create_variable_decl(context_t ctx, location_t loc,
                                      node_t identifier, node_t type,
                                      node_t expression);

/* ===== Utility ===== */

vec_t cubec_ast_create_vec(context_t ctx, bool auto_dispose);

/**
 * @brief Replace an AST node in its parent's child pointer.
 *
 * Finds old_node in its parent's children (vec or direct pointer)
 * and replaces it with new_node. Also sets new_node->parent.
 *
 * old_node is NOT automatically freed — the caller must decide
 * whether to allocator_free it.
 *
 * @return true if replacement succeeded, false if old_node has no
 *         parent or is not found in the parent's children.
 */
bool cubec_node_replace(node_t old_node, node_t new_node);

#ifdef __cplusplus
}
#endif
#endif
