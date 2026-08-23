#ifndef _H_CUBEC_RUN_RUN_
#define _H_CUBEC_RUN_RUN_
#include "engine/vm.h"
#include "core/node.h"
#ifdef __cplusplus
extern "C" {
#endif

/* ---- Expression dispatcher ---- */

/**
 * @brief Run an expression node by its kind.
 * Handles all CUBEC_NODE_EXPRESSION_* and CUBEC_NODE_LITERAL_* kinds.
 */
value_t run_expression(vm_t vm, node_t node, bool shadow);

/* ---- Literal runners ---- */

value_t run_literal_numeric(vm_t vm, node_t node, bool shadow);
value_t run_literal_string(vm_t vm, node_t node, bool shadow);
value_t run_literal_char(vm_t vm, node_t node, bool shadow);
value_t run_literal_identifier(vm_t vm, node_t node, bool shadow);
value_t run_literal_nil(vm_t vm, node_t node, bool shadow);
value_t run_literal_bool(vm_t vm, node_t node, bool shadow);
value_t run_literal_undefined(vm_t vm, node_t node, bool shadow);

/* ---- Expression runners ---- */

value_t run_expression_binary(vm_t vm, node_t node, bool shadow);
value_t run_expression_assignment(vm_t vm, node_t node, bool shadow);
value_t run_expression_deref(vm_t vm, node_t node, bool shadow);
value_t run_expression_addr(vm_t vm, node_t node, bool shadow);
value_t run_expression_member(vm_t vm, node_t node, bool shadow);
value_t run_expression_call(vm_t vm, node_t node, bool shadow);
value_t run_expression_group(vm_t vm, node_t node, bool shadow);
value_t run_expression_subscript(vm_t vm, node_t node, bool shadow);
value_t run_expression_namespace_access(vm_t vm, node_t node,
                                        bool shadow);
value_t run_expression_initialize_list(vm_t vm, node_t node, bool shadow);
value_t run_expression_typeof(vm_t vm, node_t node, bool shadow);
value_t run_expression_sizeof(vm_t vm, node_t node, bool shadow);
value_t run_expression_alignof(vm_t vm, node_t node, bool shadow);

/* ---- Ternary runner ---- */

value_t run_expression_ternary(vm_t vm, node_t node, bool shadow);

/* ---- Slice runner ---- */

value_t run_expression_slice(vm_t vm, node_t node, bool shadow);

/* ---- Wildcard runner ---- */

value_t run_expression_wildcard(vm_t vm, node_t node, bool shadow);

/* ---- Spread runner ---- */

value_t run_expression_spread(vm_t vm, node_t node, bool shadow);

/* ---- Try/Assert runners ---- */

value_t run_expression_try(vm_t vm, node_t node, bool shadow);
value_t run_expression_assert(vm_t vm, node_t node, bool shadow);

/* ---- Type declaration runners ---- */

value_t run_declaration_array(vm_t vm, node_t node, bool shadow);
value_t run_declaration_pointer(vm_t vm, node_t node, bool shadow);
value_t run_declaration_slice(vm_t vm, node_t node, bool shadow);
value_t run_declaration_qualifier(vm_t vm, node_t node, bool shadow);
value_t run_declaration_tuple(vm_t vm, node_t node, bool shadow);
value_t run_declaration_callable(vm_t vm, node_t node, bool shadow);
value_t run_declaration_function(vm_t vm, node_t node, bool shadow);
value_t run_declaration_struct(vm_t vm, node_t node, bool shadow);
value_t run_declaration_union(vm_t vm, node_t node, bool shadow);
value_t run_declaration_interface(vm_t vm, node_t node, bool shadow);

/* ---- Program runner ---- */

value_t run_program(vm_t vm, node_t node, bool shadow);

/* ---- Statement dispatcher ---- */

/**
 * @brief Run a statement node by its kind.
 * In shadow mode, errors are written to vm_get_diagnostics(vm) (not propagated)
 * and void is always returned. In script mode, errors propagate normally.
 */
value_t run_statement(vm_t vm, node_t node, bool shadow);

/* ---- Statement runners ---- */

value_t run_statement_expression(vm_t vm, node_t node, bool shadow);
value_t run_statement_block(vm_t vm, node_t node, bool shadow);
value_t run_statement_declaration(vm_t vm, node_t node, bool shadow);
value_t run_statement_declaration_type(vm_t vm, node_t node, bool shadow);
value_t run_statement_return(vm_t vm, node_t node, bool shadow);
value_t run_statement_function(vm_t vm, node_t node, bool shadow);
value_t run_statement_if(vm_t vm, node_t node, bool shadow);
value_t run_statement_struct(vm_t vm, node_t node, bool shadow);
value_t run_statement_union(vm_t vm, node_t node, bool shadow);
value_t run_statement_cunion(vm_t vm, node_t node, bool shadow);
value_t run_statement_interface(vm_t vm, node_t node, bool shadow);

/* ---- Enum runners ---- */

value_t run_statement_enum(vm_t vm, node_t node, bool shadow);
value_t run_declaration_enum(vm_t vm, node_t node, bool shadow);

#ifdef __cplusplus
}
#endif
#endif
