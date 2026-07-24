#ifndef _H_CUBEC_AST_FACTORY_
#define _H_CUBEC_AST_FACTORY_
#include "core/allocator.h"
#include "core/location.h"
#include "core/node.h"
#include "core/vec.h"
#include "cubec/literal_numeric.h"
#ifdef __cplusplus
extern "C" {
#endif

/* ===== Literals ===== */

node_t cubec_ast_create_identifier(allocator_t alloc, location_t loc,
                                   const char *name);
node_t cubec_ast_create_numeric(allocator_t alloc, location_t loc,
                                const char *value,
                                cubec_literal_numeric_kind_t kind,
                                cubec_literal_numeric_type_t ntype);
node_t cubec_ast_create_string(allocator_t alloc, location_t loc,
                               const char *value);
node_t cubec_ast_create_char(allocator_t alloc, location_t loc, char value);
node_t cubec_ast_create_undefined(allocator_t alloc, location_t loc);

/* ===== Expressions ===== */

node_t cubec_ast_create_binary(allocator_t alloc, location_t loc,
                               const char *op, node_t left, node_t right);
node_t cubec_ast_create_assignment(allocator_t alloc, location_t loc,
                                   const char *op, node_t lvalue,
                                   node_t rvalue);
node_t cubec_ast_create_call(allocator_t alloc, location_t loc,
                             node_t callee, vec_t args);
node_t cubec_ast_create_member(allocator_t alloc, location_t loc,
                               node_t host, const char *field);
node_t cubec_ast_create_namespace_access(allocator_t alloc, location_t loc,
                                         node_t host, const char *field);
node_t cubec_ast_create_deref(allocator_t alloc, location_t loc, node_t host);
node_t cubec_ast_create_addr(allocator_t alloc, location_t loc, node_t host);
node_t cubec_ast_create_try(allocator_t alloc, location_t loc, node_t host);
node_t cubec_ast_create_assert(allocator_t alloc, location_t loc, node_t host);
node_t cubec_ast_create_ternary(allocator_t alloc, location_t loc,
                                node_t cond, node_t then_branch,
                                node_t else_branch);
node_t cubec_ast_create_group(allocator_t alloc, location_t loc,
                              node_t inner);
node_t cubec_ast_create_typeof(allocator_t alloc, location_t loc,
                               node_t expr);
node_t cubec_ast_create_sizeof(allocator_t alloc, location_t loc,
                               node_t expr);
node_t cubec_ast_create_alignof(allocator_t alloc, location_t loc,
                                node_t expr);
node_t cubec_ast_create_slice_expr(allocator_t alloc, location_t loc,
                                   node_t host, node_t start,
                                   node_t length);
node_t cubec_ast_create_spread(allocator_t alloc, location_t loc,
                               node_t value);
node_t cubec_ast_create_comma(allocator_t alloc, location_t loc,
                              node_t left, node_t right);
node_t cubec_ast_create_generic_instantiation(allocator_t alloc,
                                              location_t loc,
                                              node_t callee, vec_t args);
node_t cubec_ast_create_type_qualifier(allocator_t alloc, location_t loc,
                                       node_t base, bool is_const,
                                       bool is_volatile);
node_t cubec_ast_create_initialize_list(allocator_t alloc, location_t loc,
                                        node_t type, vec_t items,
                                        bool is_field);
node_t cubec_ast_create_initialize_field(allocator_t alloc, location_t loc,
                                         const char *name, node_t value);
node_t cubec_ast_create_function_expr(allocator_t alloc, location_t loc,
                                      node_t name, vec_t captures,
                                      vec_t generic_params, vec_t args,
                                      node_t return_type, node_t body,
                                      bool is_c_variadic);

/* ===== Statements ===== */

node_t cubec_ast_create_program(allocator_t alloc, location_t loc,
                                vec_t statements);
node_t cubec_ast_create_block(allocator_t alloc, location_t loc,
                              vec_t statements);
node_t cubec_ast_create_struct_stmt(allocator_t alloc, location_t loc,
                                    const char *name, vec_t members,
                                    bool is_export, vec_t implements);
node_t cubec_ast_create_enum_stmt(allocator_t alloc, location_t loc,
                                  const char *name, vec_t items,
                                  bool is_export);
node_t cubec_ast_create_union_stmt(allocator_t alloc, location_t loc,
                                   const char *name, vec_t members,
                                   bool is_export, vec_t implements);
node_t cubec_ast_create_cunion_stmt(allocator_t alloc, location_t loc,
                                    const char *name, vec_t fields);
node_t cubec_ast_create_func_stmt(allocator_t alloc, location_t loc,
                                  const char *name, vec_t args,
                                  node_t return_type, node_t body,
                                  bool is_export, bool is_inline,
                                  bool is_extern, bool is_builtin,
                                  bool is_comptime, bool is_c_variadic);
node_t cubec_ast_create_var_decl_stmt(allocator_t alloc, location_t loc,
                                      const char *name, node_t type,
                                      node_t expr, bool is_export,
                                      bool is_extern, bool is_builtin,
                                      bool is_comptime, bool is_using);
node_t cubec_ast_create_type_alias(allocator_t alloc, location_t loc,
                                   const char *name, node_t type_value,
                                   bool is_export, bool is_builtin);
node_t cubec_ast_create_iface_stmt(allocator_t alloc, location_t loc,
                                   const char *name, vec_t members,
                                   bool is_export);
node_t cubec_ast_create_if_stmt(allocator_t alloc, location_t loc,
                                node_t cond, node_t then_branch,
                                node_t else_branch);
node_t cubec_ast_create_while_stmt(allocator_t alloc, location_t loc,
                                   node_t cond, node_t body);
node_t cubec_ast_create_do_while_stmt(allocator_t alloc, location_t loc,
                                      node_t body, node_t cond);
node_t cubec_ast_create_for_stmt(allocator_t alloc, location_t loc,
                                 node_t init, node_t cond, node_t incr,
                                 node_t body);
node_t cubec_ast_create_foreach_stmt(allocator_t alloc, location_t loc,
                                     bool is_var_decl, node_t variable,
                                     node_t var_type, node_t iterator,
                                     node_t body);
node_t cubec_ast_create_return_stmt(allocator_t alloc, location_t loc,
                                    node_t expr);
node_t cubec_ast_create_expr_stmt(allocator_t alloc, location_t loc,
                                  node_t expr);
node_t cubec_ast_create_break_stmt(allocator_t alloc, location_t loc);
node_t cubec_ast_create_continue_stmt(allocator_t alloc, location_t loc);
node_t cubec_ast_create_empty_stmt(allocator_t alloc, location_t loc);
node_t cubec_ast_create_defer_stmt(allocator_t alloc, location_t loc,
                                   vec_t captures, node_t body);
node_t cubec_ast_create_switch_stmt(allocator_t alloc, location_t loc,
                                    node_t cond, vec_t matches);
node_t cubec_ast_create_import_stmt(allocator_t alloc, location_t loc,
                                    const char *module_name,
                                    const char *alias, const char *path);
node_t cubec_ast_create_test_stmt(allocator_t alloc, location_t loc,
                                  const char *name, node_t body);

/* ===== Sub-element nodes ===== */

node_t cubec_ast_create_struct_field(allocator_t alloc, location_t loc,
                                     const char *name, node_t type,
                                     bool is_pub);
node_t cubec_ast_create_enum_item(allocator_t alloc, location_t loc,
                                  const char *name, node_t type,
                                  node_t value);
node_t cubec_ast_create_union_field(allocator_t alloc, location_t loc,
                                    const char *name, node_t type);
node_t cubec_ast_create_func_arg(allocator_t alloc, location_t loc,
                                 const char *name, node_t type);
node_t cubec_ast_create_iface_method(allocator_t alloc, location_t loc,
                                     const char *name, vec_t args,
                                     node_t return_type);
node_t cubec_ast_create_switch_match(allocator_t alloc, location_t loc,
                                     bool is_else, vec_t values,
                                     node_t body);
node_t cubec_ast_create_decorator(allocator_t alloc, location_t loc,
                                  node_t expr);
node_t cubec_ast_create_generic_param(allocator_t alloc, location_t loc,
                                      const char *name, node_t constraint,
                                      node_t value_type, bool is_rest);
node_t cubec_ast_create_func_capture(allocator_t alloc, location_t loc,
                                     const char *name);

/* ===== Type expression nodes ===== */

node_t cubec_ast_create_pointer_type(allocator_t alloc, location_t loc,
                                     node_t base, bool is_const,
                                     bool is_volatile);
node_t cubec_ast_create_slice_type(allocator_t alloc, location_t loc,
                                   node_t base, bool is_const,
                                   bool is_volatile);
node_t cubec_ast_create_array_type(allocator_t alloc, location_t loc,
                                   node_t size, node_t base);
node_t cubec_ast_create_variable_decl(allocator_t alloc, location_t loc,
                                      node_t identifier, node_t type,
                                      node_t expression);

/* ===== Utility ===== */

vec_t cubec_ast_create_vec(allocator_t alloc, bool auto_dispose);

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
