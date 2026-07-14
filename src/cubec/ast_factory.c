#include "cubec/ast_factory.h"
#include "core/string.h"
#include "cubec/program.h"
#include "cubec/literal_identifier.h"
#include "cubec/literal_string.h"
#include "cubec/literal_char.h"
#include "cubec/expression_binary.h"
#include "cubec/expression_assignment.h"
#include "cubec/expression_call.h"
#include "cubec/expression_member.h"
#include "cubec/expression_namespace_access.h"
#include "cubec/expression_postfix_unary.h"
#include "cubec/expression_ternary.h"
#include "cubec/expression_group.h"
#include "cubec/expression_typeof.h"
#include "cubec/expression_sizeof.h"
#include "cubec/expression_alignof.h"
#include "cubec/expression_slice.h"
#include "cubec/expression_spread.h"
#include "cubec/expression_comma.h"
#include "cubec/expression_generic_instantiation.h"
#include "cubec/expression_type_qualifier.h"
#include "cubec/expression_initialize_list.h"
#include "cubec/expression_initialize_field.h"
#include "cubec/expression_function.h"
#include "cubec/statement_block.h"
#include "cubec/statement_expression.h"
#include "cubec/statement_return.h"
#include "cubec/statement_if.h"
#include "cubec/statement_while.h"
#include "cubec/statement_do_while.h"
#include "cubec/statement_for.h"
#include "cubec/statement_foreach.h"
#include "cubec/statement_break.h"
#include "cubec/statement_continue.h"
#include "cubec/statement_defer.h"
#include "cubec/statement_switch.h"
#include "cubec/statement_empty.h"
#include "cubec/statement_import.h"
#include "cubec/statement_test.h"
#include "cubec/statement_struct.h"
#include "cubec/statement_enum.h"
#include "cubec/statement_union.h"
#include "cubec/statement_cunion.h"
#include "cubec/statement_function.h"
#include "cubec/statement_declaration.h"
#include "cubec/statement_declaration_type.h"
#include "cubec/statement_interface.h"
#include "cubec/struct_field.h"
#include "cubec/enum_item.h"
#include "cubec/union_field.h"
#include "cubec/function_argument.h"
#include "cubec/interface_method.h"
#include "cubec/switch_match.h"
#include "cubec/decorator.h"
#include "cubec/generic_param.h"
#include "cubec/function_capture.h"
#include "cubec/declaration_pointer.h"
#include "cubec/declaration_slice.h"
#include "cubec/declaration_array.h"
#include "cubec/declaration_variable.h"
#include "cubec/expression_type_struct.h"
#include "cubec/expression_type_enum.h"
#include "cubec/expression_type_union.h"
#include "cubec/expression_type_function.h"
#include "cubec/expression_type_interface.h"
#include "cubec/node.h"
#include <assert.h>

/* ===== Helpers ===== */

static string_t _make_string(allocator_t alloc, const char *str) {
  string_init_t si = {.str = str};
  return (string_t)allocator_create(alloc, &g_string_type, &si);
}

static cubec_literal_identifier_t _make_ident_node(allocator_t alloc,
                                                    location_t loc,
                                                    const char *name) {
  cubec_literal_identifier_init_t init = {.location = loc, .parent = NULL,
                                           .value = name};
  return (cubec_literal_identifier_t)allocator_create(
      alloc, &g_cubec_literal_identifier_type, &init);
}

/* ===== Literals ===== */

node_t cubec_ast_create_identifier(allocator_t alloc, location_t loc,
                                   const char *name) {
  return (node_t)_make_ident_node(alloc, loc, name);
}

node_t cubec_ast_create_numeric(allocator_t alloc, location_t loc,
                                const char *value,
                                cubec_literal_numeric_kind_t kind,
                                cubec_literal_numeric_type_t ntype) {
  cubec_literal_numeric_init_t init = {.location = loc, .parent = NULL,
                                       .value = value, .kind = kind,
                                       .numeric_type = ntype};
  return (node_t)allocator_create(alloc, &g_cubec_literal_numeric_type, &init);
}

node_t cubec_ast_create_string(allocator_t alloc, location_t loc,
                               const char *value) {
  cubec_literal_string_init_t init = {.location = loc, .parent = NULL,
                                       .value = value};
  return (node_t)allocator_create(alloc, &g_cubec_literal_string_type, &init);
}

node_t cubec_ast_create_char(allocator_t alloc, location_t loc, char value) {
  cubec_literal_char_init_t init = {.location = loc, .parent = NULL,
                                    .value = value};
  return (node_t)allocator_create(alloc, &g_cubec_literal_char_type, &init);
}

/* ===== Expressions ===== */

node_t cubec_ast_create_binary(allocator_t alloc, location_t loc,
                               const char *op, node_t left, node_t right) {
  string_t op_str = _make_string(alloc, op);
  cubec_expression_binary_init_t init = {.location = loc, .parent = NULL,
                                         .left = left, .right = right,
                                         .opt = op_str};
  return (node_t)allocator_create(alloc, &g_cubec_expression_binary_type,
                                  &init);
}

node_t cubec_ast_create_assignment(allocator_t alloc, location_t loc,
                                   const char *op, node_t lvalue,
                                   node_t rvalue) {
  string_t op_str = _make_string(alloc, op);
  cubec_expression_assignment_init_t init = {.location = loc, .parent = NULL,
                                              .lvalue = lvalue,
                                              .rvalue = rvalue,
                                              .opt = op_str};
  return (node_t)allocator_create(alloc, &g_cubec_expression_assignment_type,
                                  &init);
}

node_t cubec_ast_create_call(allocator_t alloc, location_t loc,
                             node_t callee, vec_t args) {
  cubec_expression_call_init_t init = {.location = loc, .parent = NULL,
                                       .callee = callee,
                                       .arguments = args};
  return (node_t)allocator_create(alloc, &g_cubec_expression_call_type,
                                  &init);
}

node_t cubec_ast_create_member(allocator_t alloc, location_t loc,
                               node_t host, const char *field) {
  cubec_expression_member_init_t init = {
      .location = loc, .parent = NULL, .host = host,
      .field = _make_ident_node(alloc, loc, field)};
  return (node_t)allocator_create(alloc, &g_cubec_expression_member_type,
                                  &init);
}

node_t cubec_ast_create_namespace_access(allocator_t alloc, location_t loc,
                                         node_t host, const char *field) {
  cubec_expression_namespace_access_init_t init = {
      .location = loc, .parent = NULL, .host = host,
      .field = _make_ident_node(alloc, loc, field)};
  return (node_t)allocator_create(
      alloc, &g_cubec_expression_namespace_access_type, &init);
}

node_t cubec_ast_create_deref(allocator_t alloc, location_t loc,
                              node_t host) {
  string_t op_str = _make_string(alloc, ".*");
  cubec_expression_postfix_unary_init_t init = {
      .location = loc, .parent = NULL, .host = host, .opt = op_str,
      .kind = CUBEC_NODE_EXPRESSION_DEREF};
  return (node_t)allocator_create(alloc, &g_cubec_expression_postfix_unary_type,
                                  &init);
}

node_t cubec_ast_create_addr(allocator_t alloc, location_t loc, node_t host) {
  string_t op_str = _make_string(alloc, ".&");
  cubec_expression_postfix_unary_init_t init = {
      .location = loc, .parent = NULL, .host = host, .opt = op_str,
      .kind = CUBEC_NODE_EXPRESSION_ADDR};
  return (node_t)allocator_create(alloc, &g_cubec_expression_postfix_unary_type,
                                  &init);
}

node_t cubec_ast_create_try(allocator_t alloc, location_t loc, node_t host) {
  string_t op_str = _make_string(alloc, ".?");
  cubec_expression_postfix_unary_init_t init = {
      .location = loc, .parent = NULL, .host = host, .opt = op_str,
      .kind = CUBEC_NODE_EXPRESSION_TRY};
  return (node_t)allocator_create(alloc, &g_cubec_expression_postfix_unary_type,
                                  &init);
}

node_t cubec_ast_create_ternary(allocator_t alloc, location_t loc,
                                node_t cond, node_t then_branch,
                                node_t else_branch) {
  cubec_expression_ternary_init_t init = {
      .location = loc, .parent = NULL, .condition = cond,
      .consequent = then_branch, .alternate = else_branch};
  return (node_t)allocator_create(alloc, &g_cubec_expression_ternary_type,
                                  &init);
}

node_t cubec_ast_create_group(allocator_t alloc, location_t loc,
                              node_t inner) {
  cubec_expression_group_init_t init = {.location = loc, .parent = NULL,
                                        .inner = inner};
  return (node_t)allocator_create(alloc, &g_cubec_expression_group_type,
                                  &init);
}

node_t cubec_ast_create_typeof(allocator_t alloc, location_t loc,
                               node_t expr) {
  cubec_expression_typeof_init_t init = {.location = loc, .parent = NULL,
                                         .expression = expr};
  return (node_t)allocator_create(alloc, &g_cubec_expression_typeof_type,
                                  &init);
}

node_t cubec_ast_create_sizeof(allocator_t alloc, location_t loc,
                               node_t expr) {
  cubec_expression_sizeof_init_t init = {.location = loc, .parent = NULL,
                                         .expression = expr};
  return (node_t)allocator_create(alloc, &g_cubec_expression_sizeof_type,
                                  &init);
}

node_t cubec_ast_create_alignof(allocator_t alloc, location_t loc,
                                node_t expr) {
  cubec_expression_alignof_init_t init = {.location = loc, .parent = NULL,
                                          .expression = expr};
  return (node_t)allocator_create(alloc, &g_cubec_expression_alignof_type,
                                  &init);
}

node_t cubec_ast_create_slice_expr(allocator_t alloc, location_t loc,
                                   node_t host, node_t start,
                                   node_t length) {
  cubec_expression_slice_init_t init = {.location = loc, .parent = NULL,
                                        .host = host, .start = start,
                                        .length = length};
  return (node_t)allocator_create(alloc, &g_cubec_expression_slice_type,
                                  &init);
}

node_t cubec_ast_create_spread(allocator_t alloc, location_t loc,
                               node_t value) {
  cubec_expression_spread_init_t init = {.location = loc, .parent = NULL,
                                         .value = value};
  return (node_t)allocator_create(alloc, &g_cubec_expression_spread_type,
                                  &init);
}

node_t cubec_ast_create_comma(allocator_t alloc, location_t loc,
                              node_t left, node_t right) {
  cubec_expression_comma_init_t init = {.location = loc, .parent = NULL,
                                        .left = left, .right = right};
  return (node_t)allocator_create(alloc, &g_cubec_expression_comma_type,
                                  &init);
}

node_t cubec_ast_create_generic_instantiation(allocator_t alloc,
                                              location_t loc,
                                              node_t callee, vec_t args) {
  cubec_expression_generic_instantiation_init_t init = {
      .location = loc, .parent = NULL, .callee = callee, .arguments = args};
  return (node_t)allocator_create(
      alloc, &g_cubec_expression_generic_instantiation_type, &init);
}

node_t cubec_ast_create_type_qualifier(allocator_t alloc, location_t loc,
                                       node_t base, bool is_volatile) {
  cubec_expression_type_qualifier_init_t init = {
      .location = loc, .parent = NULL, .type = base,
      .is_volatile = is_volatile};
  return (node_t)allocator_create(
      alloc, &g_cubec_expression_type_qualifier_type, &init);
}

node_t cubec_ast_create_initialize_list(allocator_t alloc, location_t loc,
                                        node_t type, vec_t items,
                                        bool is_field) {
  cubec_expression_initialize_list_init_t init = {
      .location = loc, .parent = NULL, .type = type, .items = items,
      .is_field = is_field};
  return (node_t)allocator_create(
      alloc, &g_cubec_expression_initialize_list_type, &init);
}

node_t cubec_ast_create_initialize_field(allocator_t alloc, location_t loc,
                                         const char *name, node_t value) {
  cubec_expression_initialize_field_init_t init = {
      .location = loc, .parent = NULL,
      .field = _make_ident_node(alloc, loc, name), .value = value};
  return (node_t)allocator_create(
      alloc, &g_cubec_expression_initialize_field_type, &init);
}

node_t cubec_ast_create_function_expr(allocator_t alloc, location_t loc,
                                      node_t name, vec_t captures,
                                      vec_t generic_params, vec_t args,
                                      node_t return_type, node_t body,
                                      bool is_c_variadic) {
  cubec_expression_function_init_t init = {
      .location = loc, .parent = NULL, .name = name,
      .captures = captures, .generic_params = generic_params,
      .arguments = args, .return_type = return_type, .body = body,
      .is_c_variadic = is_c_variadic};
  return (node_t)allocator_create(alloc, &g_cubec_expression_function_type,
                                  &init);
}

/* ===== Statements ===== */

node_t cubec_ast_create_program(allocator_t alloc, location_t loc,
                                vec_t statements) {
  cubec_program_node_init_t init = {.location = loc, .parent = NULL,
                                    .statements = statements};
  return (node_t)allocator_create(alloc, &g_cubec_program_node_type, &init);
}

node_t cubec_ast_create_block(allocator_t alloc, location_t loc,
                              vec_t statements) {
  cubec_statement_block_init_t init = {.location = loc, .parent = NULL,
                                       .statements = statements};
  return (node_t)allocator_create(alloc, &g_cubec_statement_block_type,
                                  &init);
}

node_t cubec_ast_create_struct_stmt(allocator_t alloc, location_t loc,
                                    const char *name, vec_t members,
                                    bool is_export) {
  node_t name_node = (node_t)_make_ident_node(alloc, loc, name);
  cubec_statement_struct_init_t init = {
      .location = loc, .parent = NULL, .is_export = is_export,
      .name = name_node, .generic_params = NULL, .members = members};
  return (node_t)allocator_create(alloc, &g_cubec_statement_struct_type,
                                  &init);
}

node_t cubec_ast_create_enum_stmt(allocator_t alloc, location_t loc,
                                  const char *name, vec_t items,
                                  bool is_export) {
  node_t name_node = (node_t)_make_ident_node(alloc, loc, name);
  cubec_statement_enum_init_t init = {.location = loc, .parent = NULL,
                                      .is_export = is_export,
                                      .name = name_node, .items = items};
  return (node_t)allocator_create(alloc, &g_cubec_statement_enum_type,
                                  &init);
}

node_t cubec_ast_create_union_stmt(allocator_t alloc, location_t loc,
                                   const char *name, vec_t members,
                                   bool is_export) {
  node_t name_node = (node_t)_make_ident_node(alloc, loc, name);
  cubec_statement_union_init_t init = {
      .location = loc, .parent = NULL, .is_export = is_export,
      .name = name_node, .generic_params = NULL, .members = members};
  return (node_t)allocator_create(alloc, &g_cubec_statement_union_type,
                                  &init);
}

node_t cubec_ast_create_cunion_stmt(allocator_t alloc, location_t loc,
                                    const char *name, vec_t fields) {
  node_t name_node = (node_t)_make_ident_node(alloc, loc, name);
  cubec_statement_cunion_init_t init = {
      .location = loc, .parent = NULL, .name = name_node, .fields = fields};
  return (node_t)allocator_create(alloc, &g_cubec_statement_cunion_type,
                                  &init);
}

node_t cubec_ast_create_func_stmt(allocator_t alloc, location_t loc,
                                  const char *name, vec_t args,
                                  node_t return_type, node_t body,
                                  bool is_export, bool is_inline,
                                  bool is_extern, bool is_builtin,
                                  bool is_comptime, bool is_c_variadic) {
  node_t name_node = (node_t)_make_ident_node(alloc, loc, name);
  cubec_statement_function_init_t init = {
      .location = loc, .parent = NULL, .is_export = is_export,
      .is_inline = is_inline, .is_extern = is_extern,
      .is_builtin = is_builtin, .is_comptime = is_comptime,
      .is_c_variadic = is_c_variadic, .name = name_node,
      .generic_params = NULL, .arguments = args,
      .return_type = return_type, .body = body};
  return (node_t)allocator_create(alloc, &g_cubec_statement_function_type,
                                  &init);
}

node_t cubec_ast_create_var_decl_stmt(allocator_t alloc, location_t loc,
                                      const char *name, node_t type,
                                      node_t expr, bool is_export,
                                      bool is_extern, bool is_builtin,
                                      bool is_comptime) {
  node_t name_node = (node_t)_make_ident_node(alloc, loc, name);
  cubec_declaration_variable_init_t dv_init = {
      .location = loc, .parent = NULL, .identifier = name_node,
      .type = type, .expression = expr};
  node_t decl_node = (node_t)allocator_create(
      alloc, &g_cubec_declaration_variable_type, &dv_init);
  cubec_statement_declaration_init_t sd_init = {
      .location = loc, .parent = NULL, .is_export = is_export,
      .is_extern = is_extern, .is_builtin = is_builtin,
      .is_comptime = is_comptime, .declarator = decl_node};
  return (node_t)allocator_create(alloc, &g_cubec_statement_declaration_type,
                                  &sd_init);
}

node_t cubec_ast_create_type_alias(allocator_t alloc, location_t loc,
                                   const char *name, node_t type_value,
                                   bool is_export, bool is_builtin) {
  node_t name_node = (node_t)_make_ident_node(alloc, loc, name);
  cubec_statement_declaration_type_init_t init = {
      .location = loc, .parent = NULL, .is_export = is_export,
      .is_builtin = is_builtin, .name = name_node, .params = NULL,
      .type_value = type_value};
  return (node_t)allocator_create(
      alloc, &g_cubec_statement_decltype, &init);
}

node_t cubec_ast_create_iface_stmt(allocator_t alloc, location_t loc,
                                   const char *name, vec_t members,
                                   bool is_export) {
  node_t name_node = (node_t)_make_ident_node(alloc, loc, name);
  cubec_statement_interface_init_t init = {
      .location = loc, .parent = NULL, .is_export = is_export,
      .name = name_node, .generic_params = NULL, .members = members};
  return (node_t)allocator_create(alloc, &g_cubec_statement_interface_type,
                                  &init);
}

node_t cubec_ast_create_if_stmt(allocator_t alloc, location_t loc,
                                node_t cond, node_t then_branch,
                                node_t else_branch) {
  cubec_statement_if_init_t init = {
      .location = loc, .parent = NULL, .condition = cond,
      .then_branch = then_branch, .else_branch = else_branch};
  return (node_t)allocator_create(alloc, &g_cubec_statement_if_type, &init);
}

node_t cubec_ast_create_while_stmt(allocator_t alloc, location_t loc,
                                   node_t cond, node_t body) {
  cubec_statement_while_init_t init = {.location = loc, .parent = NULL,
                                       .condition = cond, .body = body};
  return (node_t)allocator_create(alloc, &g_cubec_statement_while_type,
                                  &init);
}

node_t cubec_ast_create_do_while_stmt(allocator_t alloc, location_t loc,
                                      node_t body, node_t cond) {
  cubec_statement_do_while_init_t init = {.location = loc, .parent = NULL,
                                          .body = body, .condition = cond};
  return (node_t)allocator_create(alloc, &g_cubec_statement_do_while_type,
                                  &init);
}

node_t cubec_ast_create_for_stmt(allocator_t alloc, location_t loc,
                                 node_t init_node, node_t cond,
                                 node_t incr, node_t body) {
  cubec_statement_for_init_t init = {
      .location = loc, .parent = NULL, .init = init_node,
      .condition = cond, .increment = incr, .body = body};
  return (node_t)allocator_create(alloc, &g_cubec_statement_for_type, &init);
}

node_t cubec_ast_create_foreach_stmt(allocator_t alloc, location_t loc,
                                     bool is_const, node_t name,
                                     node_t iterator, node_t body) {
  cubec_statement_foreach_init_t init = {
      .location = loc, .parent = NULL, .is_const = is_const,
      .name = name, .iterator = iterator, .body = body};
  return (node_t)allocator_create(alloc, &g_cubec_statement_foreach_type,
                                  &init);
}

node_t cubec_ast_create_return_stmt(allocator_t alloc, location_t loc,
                                    node_t expr) {
  cubec_statement_return_init_t init = {.location = loc, .parent = NULL,
                                        .expression = expr};
  return (node_t)allocator_create(alloc, &g_cubec_statement_return_type,
                                  &init);
}

node_t cubec_ast_create_expr_stmt(allocator_t alloc, location_t loc,
                                  node_t expr) {
  cubec_statement_expression_init_t init = {.location = loc, .parent = NULL,
                                            .expression = expr};
  return (node_t)allocator_create(alloc, &g_cubec_statement_expression_type,
                                  &init);
}

node_t cubec_ast_create_break_stmt(allocator_t alloc, location_t loc) {
  cubec_statement_break_init_t init = {.location = loc, .parent = NULL};
  return (node_t)allocator_create(alloc, &g_cubec_statement_break_type,
                                  &init);
}

node_t cubec_ast_create_continue_stmt(allocator_t alloc, location_t loc) {
  cubec_statement_continue_init_t init = {.location = loc, .parent = NULL};
  return (node_t)allocator_create(alloc, &g_cubec_statement_continue_type,
                                  &init);
}

node_t cubec_ast_create_empty_stmt(allocator_t alloc, location_t loc) {
  cubec_statement_empty_init_t init = {.location = loc, .parent = NULL};
  return (node_t)allocator_create(alloc, &g_cubec_statement_empty_type,
                                  &init);
}

node_t cubec_ast_create_defer_stmt(allocator_t alloc, location_t loc,
                                   vec_t captures, node_t body) {
  cubec_statement_defer_init_t init = {.location = loc, .parent = NULL,
                                       .captures = captures, .body = body};
  return (node_t)allocator_create(alloc, &g_cubec_statement_defer_type,
                                  &init);
}

node_t cubec_ast_create_switch_stmt(allocator_t alloc, location_t loc,
                                    node_t cond, vec_t matches) {
  cubec_statement_switch_init_t init = {.location = loc, .parent = NULL,
                                        .condition = cond,
                                        .matches = matches};
  return (node_t)allocator_create(alloc, &g_cubec_statement_switch_type,
                                  &init);
}

node_t cubec_ast_create_import_stmt(allocator_t alloc, location_t loc,
                                    const char *module_name,
                                    const char *alias, const char *path) {
  node_t mod_node = (module_name)
                        ? (node_t)_make_ident_node(alloc, loc, module_name)
                        : NULL;
  node_t alias_node =
      (alias) ? (node_t)_make_ident_node(alloc, loc, alias) : NULL;
  node_t path_node = (path) ? (node_t)_make_ident_node(alloc, loc, path)
                            : NULL;
  cubec_statement_import_init_t init = {.location = loc, .parent = NULL,
                                        .module_name = mod_node,
                                        .alias = alias_node,
                                        .path = path_node};
  return (node_t)allocator_create(alloc, &g_cubec_statement_import_type,
                                  &init);
}

node_t cubec_ast_create_test_stmt(allocator_t alloc, location_t loc,
                                  const char *name, node_t body) {
  string_t name_str = _make_string(alloc, name);
  cubec_statement_test_init_t init = {.location = loc, .parent = NULL,
                                      .name = name_str, .body = body};
  return (node_t)allocator_create(alloc, &g_cubec_statement_test_type,
                                  &init);
}

/* ===== Sub-element nodes ===== */

node_t cubec_ast_create_struct_field(allocator_t alloc, location_t loc,
                                     const char *name, node_t type,
                                     bool is_pub) {
  node_t name_node = (node_t)_make_ident_node(alloc, loc, name);
  cubec_struct_field_init_t init = {.location = loc, .parent = NULL,
                                    .is_pub = is_pub, .name = name_node,
                                    .type = type};
  return (node_t)allocator_create(alloc, &g_cubec_struct_field_type, &init);
}

node_t cubec_ast_create_enum_item(allocator_t alloc, location_t loc,
                                  const char *name, node_t type,
                                  node_t value) {
  node_t name_node = (node_t)_make_ident_node(alloc, loc, name);
  cubec_enum_item_init_t init = {.location = loc, .parent = NULL,
                                 .name = name_node, .type = type,
                                 .value = value};
  return (node_t)allocator_create(alloc, &g_cubec_enum_item_type, &init);
}

node_t cubec_ast_create_union_field(allocator_t alloc, location_t loc,
                                    const char *name, node_t type) {
  node_t name_node = (node_t)_make_ident_node(alloc, loc, name);
  cubec_union_field_init_t init = {.location = loc, .parent = NULL,
                                   .name = name_node, .type = type};
  return (node_t)allocator_create(alloc, &g_cubec_union_field_type, &init);
}

node_t cubec_ast_create_func_arg(allocator_t alloc, location_t loc,
                                 const char *name, node_t type) {
  node_t name_node = (node_t)_make_ident_node(alloc, loc, name);
  cubec_function_argument_init_t init = {.identifier = name_node,
                                         .type = type};
  return (node_t)allocator_create(alloc, &g_cubec_function_argument_type,
                                  &init);
}

node_t cubec_ast_create_iface_method(allocator_t alloc, location_t loc,
                                     const char *name, vec_t args,
                                     node_t return_type) {
  node_t name_node = (node_t)_make_ident_node(alloc, loc, name);
  cubec_interface_method_init_t init = {
      .location = loc, .name = name_node, .generic_params = NULL,
      .arguments = args, .return_type = return_type};
  return (node_t)allocator_create(alloc, &g_cubec_interface_method_type,
                                  &init);
}

node_t cubec_ast_create_switch_match(allocator_t alloc, location_t loc,
                                     bool is_else, vec_t values,
                                     node_t body) {
  cubec_switch_match_init_t init = {.location = loc, .parent = NULL,
                                    .is_else = is_else, .values = values,
                                    .body = body};
  return (node_t)allocator_create(alloc, &g_cubec_switch_match_type, &init);
}

node_t cubec_ast_create_decorator(allocator_t alloc, location_t loc,
                                  node_t expr) {
  cubec_decorator_init_t init = {.location = loc, .parent = NULL,
                                 .expression = expr};
  return (node_t)allocator_create(alloc, &g_cubec_decorator_type, &init);
}

node_t cubec_ast_create_generic_param(allocator_t alloc, location_t loc,
                                      const char *name, node_t constraint,
                                      node_t value_type, bool is_rest) {
  node_t name_node = (node_t)_make_ident_node(alloc, loc, name);
  cubec_generic_param_init_t init = {.name = name_node,
                                     .constraint = constraint,
                                     .value_type = value_type,
                                     .is_rest = is_rest};
  return (node_t)allocator_create(alloc, &g_cubec_generic_param_type, &init);
}

node_t cubec_ast_create_func_capture(allocator_t alloc, location_t loc,
                                     const char *name) {
  node_t name_node = (node_t)_make_ident_node(alloc, loc, name);
  cubec_function_capture_init_t init = {.location = loc,
                                        .identifier = name_node};
  return (node_t)allocator_create(alloc, &g_cubec_function_capture_type,
                                  &init);
}

/* ===== Type expression nodes ===== */

node_t cubec_ast_create_pointer_type(allocator_t alloc, location_t loc,
                                     node_t base, bool is_const,
                                     bool is_volatile) {
  cubec_declaration_pointer_init_t init = {.location = loc, .parent = NULL,
                                           .type = base,
                                           .is_const = is_const,
                                           .is_volatile = is_volatile};
  return (node_t)allocator_create(alloc, &g_cubec_declaration_pointer_type,
                                  &init);
}

node_t cubec_ast_create_slice_type(allocator_t alloc, location_t loc,
                                   node_t base, bool is_const,
                                   bool is_volatile) {
  cubec_declaration_slice_init_t init = {.location = loc, .parent = NULL,
                                         .type = base,
                                         .is_const = is_const,
                                         .is_volatile = is_volatile};
  return (node_t)allocator_create(alloc, &g_cubec_declaration_slice_type,
                                  &init);
}

node_t cubec_ast_create_array_type(allocator_t alloc, location_t loc,
                                   node_t size, node_t base) {
  cubec_declaration_array_init_t init = {.location = loc, .parent = NULL,
                                         .size = size, .type = base};
  return (node_t)allocator_create(alloc, &g_cubec_declaration_array_type,
                                  &init);
}

node_t cubec_ast_create_variable_decl(allocator_t alloc, location_t loc,
                                      node_t identifier, node_t type,
                                      node_t expression) {
  cubec_declaration_variable_init_t init = {
      .location = loc, .parent = NULL, .identifier = identifier,
      .type = type, .expression = expression};
  return (node_t)allocator_create(alloc, &g_cubec_declaration_variable_type,
                                  &init);
}

/* ===== Utility ===== */

vec_t cubec_ast_create_vec(allocator_t alloc, bool auto_dispose) {
  vec_init_t vi = {.auto_dispose = auto_dispose};
  return (vec_t)allocator_create(alloc, &g_vec_type, &vi);
}

/* ===== Node replacement ===== */

static bool _replace_in_vec(vec_t vec, node_t old_node, node_t new_node) {
  if (!vec) return false;
  size_t size = vec_get_size(vec);
  for (size_t i = 0; i < size; i++) {
    if (vec_get(vec, i) == old_node) {
      /* auto_dispose vec will free old_node when replaced — but we don't
         want that, so we set the slot directly without going through
         vec_set (which would dispose the old value). Since vec stores
         raw pointers, we can overwrite the slot. */
      void **slots = (void **)vec_get_data(vec);
      slots[i] = new_node;
      new_node->parent = old_node->parent;
      return true;
    }
  }
  return false;
}

static bool _replace_ptr(node_t *slot, node_t old_node, node_t new_node) {
  if (*slot == old_node) {
    *slot = new_node;
    new_node->parent = old_node->parent;
    return true;
  }
  return false;
}

bool cubec_node_replace(node_t old_node, node_t new_node) {
  if (!old_node || !old_node->parent || !new_node) return false;
  node_t parent = old_node->parent;

  switch (parent->kind) {
  /* Expressions with node_t children */
  case CUBEC_NODE_EXPRESSION_BINARY:
  case CUBEC_NODE_EXPRESSION_ASSIGNMENT: {
    cubec_expression_binary_t bin = (cubec_expression_binary_t)parent;
    return _replace_ptr(&bin->left, old_node, new_node) ||
           _replace_ptr(&bin->right, old_node, new_node);
  }
  case CUBEC_NODE_EXPRESSION_CALL: {
    cubec_expression_call_t call = (cubec_expression_call_t)parent;
    return _replace_ptr(&call->callee, old_node, new_node) ||
           _replace_in_vec(call->arguments, old_node, new_node);
  }
  case CUBEC_NODE_EXPRESSION_MEMBER: {
    cubec_expression_member_t mem = (cubec_expression_member_t)parent;
    return _replace_ptr(&mem->host, old_node, new_node);
  }
  case CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS: {
    cubec_expression_namespace_access_t na =
        (cubec_expression_namespace_access_t)parent;
    return _replace_ptr(&na->host, old_node, new_node);
  }
  case CUBEC_NODE_EXPRESSION_TERNARY: {
    cubec_expression_ternary_t ter = (cubec_expression_ternary_t)parent;
    return _replace_ptr(&ter->condition, old_node, new_node) ||
           _replace_ptr(&ter->consequent, old_node, new_node) ||
           _replace_ptr(&ter->alternate, old_node, new_node);
  }
  case CUBEC_NODE_EXPRESSION_GROUP: {
    cubec_expression_group_t grp = (cubec_expression_group_t)parent;
    return _replace_ptr(&grp->inner, old_node, new_node);
  }
  case CUBEC_NODE_EXPRESSION_TYPEOF: {
    cubec_expression_typeof_t to = (cubec_expression_typeof_t)parent;
    return _replace_ptr(&to->expression, old_node, new_node);
  }
  case CUBEC_NODE_EXPRESSION_SIZEOF: {
    cubec_expression_sizeof_t so = (cubec_expression_sizeof_t)parent;
    return _replace_ptr(&so->expression, old_node, new_node);
  }
  case CUBEC_NODE_EXPRESSION_ALIGNOF: {
    cubec_expression_alignof_t ao = (cubec_expression_alignof_t)parent;
    return _replace_ptr(&ao->expression, old_node, new_node);
  }
  case CUBEC_NODE_EXPRESSION_SLICE: {
    cubec_expression_slice_t sl = (cubec_expression_slice_t)parent;
    return _replace_ptr(&sl->host, old_node, new_node) ||
           _replace_ptr(&sl->start, old_node, new_node) ||
           _replace_ptr(&sl->length, old_node, new_node);
  }
  case CUBEC_NODE_EXPRESSION_SPREAD: {
    cubec_expression_spread_t sp = (cubec_expression_spread_t)parent;
    return _replace_ptr(&sp->value, old_node, new_node);
  }
  case CUBEC_NODE_EXPRESSION_COMMA: {
    cubec_expression_comma_t cm = (cubec_expression_comma_t)parent;
    return _replace_ptr(&cm->left, old_node, new_node) ||
           _replace_ptr(&cm->right, old_node, new_node);
  }
  case CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION: {
    cubec_expression_generic_instantiation_t gi =
        (cubec_expression_generic_instantiation_t)parent;
    return _replace_ptr(&gi->callee, old_node, new_node) ||
           _replace_in_vec(gi->arguments, old_node, new_node);
  }
  case CUBEC_NODE_EXPRESSION_TYPE_QUALIFIER: {
    cubec_expression_type_qualifier_t tq =
        (cubec_expression_type_qualifier_t)parent;
    return _replace_ptr(&tq->type, old_node, new_node);
  }
  case CUBEC_NODE_EXPRESSION_INITIALIZE_LIST: {
    cubec_expression_initialize_list_t il =
        (cubec_expression_initialize_list_t)parent;
    return _replace_ptr(&il->type, old_node, new_node) ||
           _replace_in_vec(il->items, old_node, new_node);
  }
  case CUBEC_NODE_EXPRESSION_INITIALIZE_FIELD: {
    cubec_expression_initialize_field_t if_ =
        (cubec_expression_initialize_field_t)parent;
    return _replace_ptr(&if_->value, old_node, new_node);
  }
  case CUBEC_NODE_EXPRESSION_FUNCTION: {
    cubec_expression_function_t fn =
        (cubec_expression_function_t)parent;
    return _replace_ptr(&fn->return_type, old_node, new_node) ||
           _replace_ptr(&fn->body, old_node, new_node) ||
           _replace_in_vec(fn->arguments, old_node, new_node) ||
           _replace_in_vec(fn->captures, old_node, new_node) ||
           _replace_in_vec(fn->generic_params, old_node, new_node);
  }

  /* Postfix unary: deref, addr, try (reuses binary struct: left=NULL, right=host) */
  case CUBEC_NODE_EXPRESSION_DEREF:
  case CUBEC_NODE_EXPRESSION_ADDR:
  case CUBEC_NODE_EXPRESSION_TRY: {
    cubec_expression_postfix_unary_t pu =
        (cubec_expression_postfix_unary_t)parent;
    return _replace_ptr(&pu->left, old_node, new_node) ||
           _replace_ptr(&pu->right, old_node, new_node);
  }

  /* Statements with node_t children */
  case CUBEC_NODE_PROGRAM: {
    cubec_program_node_t prog = (cubec_program_node_t)parent;
    return _replace_in_vec(prog->statements, old_node, new_node);
  }
  case CUBEC_NODE_STATEMENT_BLOCK: {
    cubec_statement_block_t blk = (cubec_statement_block_t)parent;
    return _replace_in_vec(blk->statements, old_node, new_node);
  }
  case CUBEC_NODE_STATEMENT_RETURN: {
    cubec_statement_return_t ret = (cubec_statement_return_t)parent;
    return _replace_ptr(&ret->expression, old_node, new_node);
  }
  case CUBEC_NODE_STATEMENT_EXPRESSION: {
    cubec_statement_expression_t se = (cubec_statement_expression_t)parent;
    return _replace_ptr(&se->expression, old_node, new_node);
  }
  case CUBEC_NODE_STATEMENT_IF: {
    cubec_statement_if_t si = (cubec_statement_if_t)parent;
    return _replace_ptr(&si->condition, old_node, new_node) ||
           _replace_ptr(&si->then_branch, old_node, new_node) ||
           _replace_ptr(&si->else_branch, old_node, new_node);
  }
  case CUBEC_NODE_STATEMENT_WHILE: {
    cubec_statement_while_t sw = (cubec_statement_while_t)parent;
    return _replace_ptr(&sw->condition, old_node, new_node) ||
           _replace_ptr(&sw->body, old_node, new_node);
  }
  case CUBEC_NODE_STATEMENT_DO_WHILE: {
    cubec_statement_do_while_t sdw = (cubec_statement_do_while_t)parent;
    return _replace_ptr(&sdw->body, old_node, new_node) ||
           _replace_ptr(&sdw->condition, old_node, new_node);
  }
  case CUBEC_NODE_STATEMENT_FOR: {
    cubec_statement_for_t sf = (cubec_statement_for_t)parent;
    return _replace_ptr(&sf->init, old_node, new_node) ||
           _replace_ptr(&sf->condition, old_node, new_node) ||
           _replace_ptr(&sf->increment, old_node, new_node) ||
           _replace_ptr(&sf->body, old_node, new_node);
  }
  case CUBEC_NODE_STATEMENT_FOREACH: {
    cubec_statement_foreach_t sfe = (cubec_statement_foreach_t)parent;
    return _replace_ptr(&sfe->name, old_node, new_node) ||
           _replace_ptr(&sfe->iterator, old_node, new_node) ||
           _replace_ptr(&sfe->body, old_node, new_node);
  }
  case CUBEC_NODE_STATEMENT_DEFER: {
    cubec_statement_defer_t sd = (cubec_statement_defer_t)parent;
    return _replace_ptr(&sd->body, old_node, new_node);
  }
  case CUBEC_NODE_STATEMENT_SWITCH: {
    cubec_statement_switch_t ss = (cubec_statement_switch_t)parent;
    return _replace_ptr(&ss->condition, old_node, new_node) ||
           _replace_in_vec(ss->matches, old_node, new_node);
  }

  /* Statement declarations */
  case CUBEC_NODE_STATEMENT_DECLARATION: {
    cubec_statement_declaration_t sdecl =
        (cubec_statement_declaration_t)parent;
    return _replace_ptr(&sdecl->declarator, old_node, new_node);
  }
  case CUBEC_NODE_STATEMENT_DECLARATION_TYPE: {
    cubec_statement_declaration_type_t sdt =
        (cubec_statement_declaration_type_t)parent;
    return _replace_ptr(&sdt->type_value, old_node, new_node) ||
           _replace_in_vec(sdt->params, old_node, new_node);
  }

  /* Struct/Enum/Union/Interface statements */
  case CUBEC_NODE_STATEMENT_STRUCT: {
    cubec_statement_struct_t ss = (cubec_statement_struct_t)parent;
    return _replace_in_vec(ss->members, old_node, new_node);
  }
  case CUBEC_NODE_STATEMENT_ENUM: {
    cubec_statement_enum_t se = (cubec_statement_enum_t)parent;
    return _replace_in_vec(se->items, old_node, new_node);
  }
  case CUBEC_NODE_STATEMENT_UNION: {
    cubec_statement_union_t su = (cubec_statement_union_t)parent;
    return _replace_in_vec(su->members, old_node, new_node);
  }
  case CUBEC_NODE_STATEMENT_CUNION: {
    cubec_statement_cunion_t sc = (cubec_statement_cunion_t)parent;
    return _replace_in_vec(sc->fields, old_node, new_node);
  }
  case CUBEC_NODE_STATEMENT_FUNCTION: {
    cubec_statement_function_t sf = (cubec_statement_function_t)parent;
    return _replace_ptr(&sf->body, old_node, new_node) ||
           _replace_in_vec(sf->arguments, old_node, new_node);
  }
  case CUBEC_NODE_STATEMENT_INTERFACE: {
    cubec_statement_interface_t si = (cubec_statement_interface_t)parent;
    return _replace_in_vec(si->members, old_node, new_node);
  }
  case CUBEC_NODE_STATEMENT_IMPORT: {
    cubec_statement_import_t si = (cubec_statement_import_t)parent;
    return _replace_ptr(&si->module_name, old_node, new_node) ||
           _replace_ptr(&si->alias, old_node, new_node) ||
           _replace_ptr(&si->path, old_node, new_node);
  }
  case CUBEC_NODE_STATEMENT_TEST: {
    cubec_statement_test_t st = (cubec_statement_test_t)parent;
    return _replace_ptr(&st->body, old_node, new_node);
  }

  /* Sub-element nodes */
  case CUBEC_NODE_STRUCT_FIELD: {
    cubec_struct_field_t sf = (cubec_struct_field_t)parent;
    return _replace_ptr(&sf->type, old_node, new_node);
  }
  case CUBEC_NODE_ENUM_ITEM: {
    cubec_enum_item_t ei = (cubec_enum_item_t)parent;
    return _replace_ptr(&ei->type, old_node, new_node) ||
           _replace_ptr(&ei->value, old_node, new_node);
  }
  case CUBEC_NODE_UNION_FIELD: {
    cubec_union_field_t uf = (cubec_union_field_t)parent;
    return _replace_ptr(&uf->type, old_node, new_node);
  }
  case CUBEC_NODE_INTERFACE_METHOD: {
    cubec_interface_method_t im = (cubec_interface_method_t)parent;
    return _replace_in_vec(im->arguments, old_node, new_node) ||
           _replace_ptr(&im->return_type, old_node, new_node);
  }
  case CUBEC_NODE_SWITCH_MATCH: {
    cubec_switch_match_t sm = (cubec_switch_match_t)parent;
    return _replace_in_vec(sm->values, old_node, new_node) ||
           _replace_ptr(&sm->body, old_node, new_node);
  }
  case CUBEC_NODE_DECORATOR: {
    cubec_decorator_t dec = (cubec_decorator_t)parent;
    return _replace_ptr(&dec->expression, old_node, new_node);
  }
  case CUBEC_NODE_GENERIC_PARAM: {
    cubec_generic_param_t gp = (cubec_generic_param_t)parent;
    return _replace_ptr(&gp->constraint, old_node, new_node) ||
           _replace_ptr(&gp->value_type, old_node, new_node);
  }

  /* Type expression nodes */
  case CUBEC_NODE_DECLARATION_POINTER: {
    cubec_declaration_pointer_t dp = (cubec_declaration_pointer_t)parent;
    return _replace_ptr(&dp->type, old_node, new_node);
  }
  case CUBEC_NODE_DECLARATION_SLICE: {
    cubec_declaration_slice_t ds = (cubec_declaration_slice_t)parent;
    return _replace_ptr(&ds->type, old_node, new_node);
  }
  case CUBEC_NODE_DECLARATION_ARRAY: {
    cubec_declaration_array_t da = (cubec_declaration_array_t)parent;
    return _replace_ptr(&da->size, old_node, new_node) ||
           _replace_ptr(&da->type, old_node, new_node);
  }
  case CUBEC_NODE_DECLARATION_VARIABLE: {
    cubec_declaration_variable_t dv = (cubec_declaration_variable_t)parent;
    return _replace_ptr(&dv->type, old_node, new_node) ||
           _replace_ptr(&dv->expression, old_node, new_node);
  }
  case CUBEC_NODE_EXPRESSION_TYPE_STRUCT: {
    cubec_expression_type_struct_t ets =
        (cubec_expression_type_struct_t)parent;
    return _replace_in_vec(ets->members, old_node, new_node);
  }
  case CUBEC_NODE_EXPRESSION_TYPE_ENUM: {
    cubec_expression_type_enum_t ete =
        (cubec_expression_type_enum_t)parent;
    return _replace_in_vec(ete->items, old_node, new_node);
  }
  case CUBEC_NODE_EXPRESSION_TYPE_UNION: {
    cubec_expression_type_union_t etu =
        (cubec_expression_type_union_t)parent;
    return _replace_in_vec(etu->members, old_node, new_node);
  }
  case CUBEC_NODE_EXPRESSION_TYPE_FUNCTION: {
    cubec_expression_type_function_t etf =
        (cubec_expression_type_function_t)parent;
    return _replace_in_vec(etf->parameters, old_node, new_node) ||
           _replace_ptr(&etf->return_type, old_node, new_node);
  }
  case CUBEC_NODE_EXPRESSION_TYPE_INTERFACE: {
    cubec_expression_type_interface_t eti =
        (cubec_expression_type_interface_t)parent;
    return _replace_in_vec(eti->members, old_node, new_node);
  }

  /* Nodes without relevant child pointers or not applicable */
  case CUBEC_NODE_LITERAL_IDENTIFIER:
  case CUBEC_NODE_LITERAL_NUMERIC:
  case CUBEC_NODE_LITERAL_STRING:
  case CUBEC_NODE_LITERAL_CHAR:
  case CUBEC_NODE_STATEMENT_BREAK:
  case CUBEC_NODE_STATEMENT_CONTINUE:
  case CUBEC_NODE_STATEMENT_EMPTY:
  case CUBEC_NODE_STATEMENT_COMPTIME_BLOCK:
  case CUBEC_NODE_STATEMENT_COMPTIME_FOR:
  case CUBEC_NODE_STATEMENT_COMPTIME_IF:
  case CUBEC_NODE_FUNCTION_ARGUMENT:
  case CUBEC_NODE_FUNCTION_BINDING:
  case CUBEC_NODE_INITIALIZE_LIST_FIELD:
  case CUBEC_NODE_CALLABLE_ARGUMENT:
  case CUBEC_NODE_DECLARATION_ENUM:
  case CUBEC_NODE_DECLARATION_INITIALIZE_LIST:
  case CUBEC_NODE_DECLARATION_STRUCT:
  case CUBEC_NODE_DECLARATION_FUNCTION:
    return false;

  default:
    assert(false && "cubec_node_replace: unhandled node kind");
    return false;
  }
}
