#include "runtime/vm.h"
#include "ast/node.h"
#include "ast/program.h"
#include "core/allocator.h"
#include "core/list.h"
#include "engine/context.h"
#include "runtime/error.h"
#include "runtime/program.h"
static void cubec_vm_dispose(cubec_vm_t self, cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->stack);
}
cubec_vm_t cubec_create_vm(cubec_allocator_t allocaor) {
  cubec_vm_t self = cubec_allocator_alloc(allocaor, sizeof(struct _cubec_vm_t),
                                          (cubec_dispose_fn_t)cubec_vm_dispose);
  self->stack = cubec_create_list(allocaor, NULL);
  return self;
}
cubec_value_t cubec_vm_run(cubec_vm_t self, cubec_context_t ctx,
                           cubec_ast_node_t node) {
  switch (node->type) {
  case CUBEC_NODE_TYPE_ERROR:
    return cubec_run_error_node(ctx, self, (cubec_ast_error_t)node);
  case CUBEC_NODE_TYPE_PROGRAM:
    return cubec_run_program_node(ctx, self, (cubec_ast_program_t)node);
  case CUBEC_NODE_TYPE_LITERAL_IDENTIFIER:
  case CUBEC_NODE_TYPE_LITERAL_NUMERIC:
  case CUBEC_NODE_TYPE_LITERAL_STRING:
  case CUBEC_NODE_TYPE_LITERAL_SYMBOL:
  case CUBEC_NODE_TYPE_LITERAL_CHAR:
  case CUBEC_NODE_TYPE_STATEMENT_EMPTY:
  case CUBEC_NODE_TYPE_STATEMENT_BLOCK:
  case CUBEC_NODE_TYPE_STATEMENT_IMPORT:
  case CUBEC_NODE_TYPE_IMPORT_DECLARATOR:
  case CUBEC_NODE_TYPE_IMPORT_NAMESPACE:
  case CUBEC_NODE_TYPE_STATEMENT_FUNCTION:
  case CUBEC_NODE_TYPE_STATEMENT_STRUCT:
  case CUBEC_NODE_TYPE_STATEMENT_ENUM:
  case CUBEC_NODE_TYPE_ENUM_FIELD:
  case CUBEC_NODE_TYPE_STATEMENT_DECLARATION:
  case CUBEC_NODE_TYPE_VARIABLE_DECLARATOR:
  case CUBEC_NODE_TYPE_STATEMENT_EXPRESSION:
  case CUBEC_NODE_TYPE_STATEMENT_IF:
  case CUBEC_NODE_TYPE_STATEMENT_SWITCH:
  case CUBEC_NODE_TYPE_SWITCH_CASE:
  case CUBEC_NODE_TYPE_STATEMENT_WHILE:
  case CUBEC_NODE_TYPE_STATEMENT_DO_WHILE:
  case CUBEC_NODE_TYPE_STATEMENT_FOR:
  case CUBEC_NODE_TYPE_STATEMENT_FOREACH:
  case CUBEC_NODE_TYPE_STATEMENT_DEFER:
  case CUBEC_NODE_TYPE_STATEMENT_BREAK:
  case CUBEC_NODE_TYPE_STATEMENT_CONTINUE:
  case CUBEC_NODE_TYPE_STATEMENT_RETURN:
  case CUBEC_NODE_TYPE_STATEMENT_TEST:
  case CUBEC_NODE_TYPE_ARRAY_DECLARATOR:
  case CUBEC_NODE_TYPE_EXPRESSION_ASSIGMENT:
  case CUBEC_NODE_TYPE_EXPRESSION_BINARY:
  case CUBEC_NODE_TYPE_EXPRESSION_CALL:
  case CUBEC_NODE_TYPE_EXPRESSION_MEMBER:
  case CUBEC_NODE_TYPE_EXPRESSION_COMPUTE_MEMBER:
  case CUBEC_NODE_TYPE_EXPRESSION_TEMPLATE_GENERATOR:
  case CUBEC_NODE_TYPE_EXPRESSION_CONDITION:
  case CUBEC_NODE_TYPE_EXPRESSION_COMMON:
  case CUBEC_NODE_TYPE_EXPRESSION_GROUP:
  case CUBEC_NODE_TYPE_EXPRESSION_SPREAD:
  case CUBEC_NODE_TYPE_STRUCT_DECLARATOR:
  case CUBEC_NODE_TYPE_STRUCT_FIELD:
  case CUBEC_NODE_TYPE_ENUM_DECLARATOR:
  case CUBEC_NODE_TYPE_FUNCTION_DECLARATOR:
  case CUBEC_NODE_TYPE_FUNCTION_BODY:
  case CUBEC_NODE_TYPE_FUNCTION_ARGUMENT:
  case CUBEC_NODE_TYPE_FUNCTION_ARGUMENT_REST:
  case CUBEC_NODE_TYPE_EXPRESSION_SLICE:
  case CUBEC_NODE_TYPE_INITIALIZE_LIST:
  case CUBEC_NODE_TYPE_INITIALIZE_FIELD:
  case CUBEC_NODE_TYPE_DECORATOR:
  case CUBEC_NODE_TYPE_INTERFACE_DECLARATOR:
  case CUBEC_NODE_TYPE_PTR_DECLARATOR:
  case CUBEC_NODE_TYPE_TYPE:
  default:
    return cubec_context_create_error(ctx, "Not implement", NULL);
  }
  return cubec_context_create_undefined(ctx, NULL);
}