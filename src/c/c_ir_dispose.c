#include "c/c_ir.h"
#include "c/c_ir_unit.h"
#include "c/c_ir_include.h"
#include "c/c_ir_typedef.h"
#include "c/c_ir_forward_decl.h"
#include "c/c_ir_function.h"
#include "c/c_ir_variable.h"
#include "c/c_ir_enum.h"
#include "c/c_ir_stmt_block.h"
#include "c/c_ir_stmt_expr.h"
#include "c/c_ir_stmt_return.h"
#include "c/c_ir_stmt_if.h"
#include "c/c_ir_stmt_while.h"
#include "c/c_ir_stmt_do_while.h"
#include "c/c_ir_stmt_for.h"
#include "c/c_ir_stmt_jump.h"
#include "c/c_ir_stmt_local_decl.h"
#include "c/c_ir_stmt_stmt_expr.h"
#include "c/c_ir_expr_binary.h"
#include "c/c_ir_expr_unary.h"
#include "c/c_ir_expr_call.h"
#include "c/c_ir_expr_member.h"
#include "c/c_ir_expr_subscript.h"
#include "c/c_ir_expr_cast.h"
#include "c/c_ir_expr_ternary.h"
#include "c/c_ir_expr_compound.h"
#include "c/c_ir_expr_sizeof.h"
#include "c/c_ir_expr_alignof.h"
#include "c/c_ir_expr_literal.h"
#include "c/c_ir_expr_initializer.h"

void c_ir_dispose_vec(allocator_t allocator, vec_t *vec) {
  if (!vec || !*vec) return;
  vec_t v = *vec;
  size_t size = vec_get_size(v);
  for (size_t i = 0; i < size; i++) {
    c_ir_node_t child = vec_get(v, i);
    c_ir_dispose(allocator, &child);
  }
  allocator_free(allocator, vec);
}

void c_ir_dispose(allocator_t allocator, c_ir_node_t *node) {
  if (!node || !*node) return;
  enum c_ir_kind kind = c_ir_get_kind(*node);

  switch (kind) {
  case C_IR_UNIT:
    c_ir_unit_dispose(allocator, (c_ir_unit_t *)node);
    return;
  case C_IR_INCLUDE:
    c_ir_include_dispose(allocator, (c_ir_include_t *)node);
    return;
  case C_IR_TYPEDEF:
    c_ir_typedef_dispose(allocator, (c_ir_typedef_t *)node);
    return;
  case C_IR_FORWARD_DECL:
    c_ir_forward_decl_dispose(allocator, (c_ir_forward_decl_t *)node);
    return;
  case C_IR_FUNCTION_DECL:
    c_ir_function_decl_dispose(allocator, (c_ir_function_decl_t *)node);
    return;
  case C_IR_FUNCTION_DEF:
    c_ir_function_def_dispose(allocator, (c_ir_function_def_t *)node);
    return;
  case C_IR_VARIABLE_DECL:
    c_ir_variable_decl_dispose(allocator, (c_ir_variable_decl_t *)node);
    return;
  case C_IR_ENUM_DEF:
    c_ir_enum_def_dispose(allocator, (c_ir_enum_def_t *)node);
    return;
  case C_IR_STMT_BLOCK:
    c_ir_stmt_block_dispose(allocator, (c_ir_stmt_block_t *)node);
    return;
  case C_IR_STMT_EXPR:
    c_ir_stmt_expr_dispose(allocator, (c_ir_stmt_expr_t *)node);
    return;
  case C_IR_STMT_RETURN:
    c_ir_stmt_return_dispose(allocator, (c_ir_stmt_return_t *)node);
    return;
  case C_IR_STMT_IF:
    c_ir_stmt_if_dispose(allocator, (c_ir_stmt_if_t *)node);
    return;
  case C_IR_STMT_WHILE:
    c_ir_stmt_while_dispose(allocator, (c_ir_stmt_while_t *)node);
    return;
  case C_IR_STMT_DO_WHILE:
    c_ir_stmt_do_while_dispose(allocator, (c_ir_stmt_do_while_t *)node);
    return;
  case C_IR_STMT_FOR:
    c_ir_stmt_for_dispose(allocator, (c_ir_stmt_for_t *)node);
    return;
  case C_IR_STMT_BREAK:
    c_ir_stmt_break_dispose(allocator, (c_ir_stmt_break_t *)node);
    return;
  case C_IR_STMT_CONTINUE:
    c_ir_stmt_continue_dispose(allocator, (c_ir_stmt_continue_t *)node);
    return;
  case C_IR_STMT_GOTO:
    c_ir_stmt_goto_dispose(allocator, (c_ir_stmt_goto_t *)node);
    return;
  case C_IR_STMT_LABEL:
    c_ir_stmt_label_dispose(allocator, (c_ir_stmt_label_t *)node);
    return;
  case C_IR_STMT_LOCAL_DECL:
    c_ir_stmt_local_decl_dispose(allocator, (c_ir_stmt_local_decl_t *)node);
    return;
  case C_IR_STMT_STMT_EXPR:
    c_ir_stmt_stmt_expr_dispose(allocator, (c_ir_stmt_stmt_expr_t *)node);
    return;
  case C_IR_EXPR_BINARY:
    c_ir_expr_binary_dispose(allocator, (c_ir_expr_binary_t *)node);
    return;
  case C_IR_EXPR_UNARY:
    c_ir_expr_unary_dispose(allocator, (c_ir_expr_unary_t *)node);
    return;
  case C_IR_EXPR_CALL:
    c_ir_expr_call_dispose(allocator, (c_ir_expr_call_t *)node);
    return;
  case C_IR_EXPR_MEMBER:
    c_ir_expr_member_dispose(allocator, (c_ir_expr_member_t *)node);
    return;
  case C_IR_EXPR_SUBSCRIPT:
    c_ir_expr_subscript_dispose(allocator, (c_ir_expr_subscript_t *)node);
    return;
  case C_IR_EXPR_CAST:
    c_ir_expr_cast_dispose(allocator, (c_ir_expr_cast_t *)node);
    return;
  case C_IR_EXPR_TERNARY:
    c_ir_expr_ternary_dispose(allocator, (c_ir_expr_ternary_t *)node);
    return;
  case C_IR_EXPR_COMPOUND:
    c_ir_expr_compound_dispose(allocator, (c_ir_expr_compound_t *)node);
    return;
  case C_IR_EXPR_SIZEOF:
    c_ir_expr_sizeof_dispose(allocator, (c_ir_expr_sizeof_t *)node);
    return;
  case C_IR_EXPR_ALIGNOF:
    c_ir_expr_alignof_dispose(allocator, (c_ir_expr_alignof_t *)node);
    return;
  case C_IR_EXPR_STRING:
    c_ir_expr_string_dispose(allocator, (c_ir_expr_string_t *)node);
    return;
  case C_IR_EXPR_NUMERIC:
    c_ir_expr_numeric_dispose(allocator, (c_ir_expr_numeric_t *)node);
    return;
  case C_IR_EXPR_CHAR:
    c_ir_expr_char_dispose(allocator, (c_ir_expr_char_t *)node);
    return;
  case C_IR_EXPR_IDENT:
    c_ir_expr_ident_dispose(allocator, (c_ir_expr_ident_t *)node);
    return;
  case C_IR_EXPR_NULL:
    c_ir_expr_null_dispose(allocator, (c_ir_expr_null_t *)node);
    return;
  case C_IR_EXPR_BOOL:
    c_ir_expr_bool_dispose(allocator, (c_ir_expr_bool_t *)node);
    return;
  case C_IR_EXPR_INITIALIZER:
    c_ir_expr_initializer_dispose(allocator, (c_ir_expr_initializer_t *)node);
    return;
  }
  abort(); /* Unknown kind */
}
