#include "c/lower.h"
#include "c/c_ir.h"
#include "c/c_type.h"
#include "c/mangle.h"
#include "c/c_ir_unit.h"
#include "c/c_ir_function.h"
#include "c/c_ir_variable.h"
#include "c/c_ir_enum.h"
#include "c/c_ir_typedef.h"
#include "c/c_ir_forward_decl.h"
#include "c/c_ir_include.h"
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
#include "cubec/node.h"
#include "cubec/statement_block.h"
#include "cubec/statement_return.h"
#include "cubec/statement_if.h"
#include "cubec/statement_while.h"
#include "cubec/statement_do_while.h"
#include "cubec/statement_for.h"
#include "cubec/statement_declaration.h"
#include "cubec/statement_function.h"
#include "cubec/statement_enum.h"
#include "cubec/statement_struct.h"
#include "cubec/statement_union.h"
#include "cubec/statement_cunion.h"
#include "cubec/statement_defer.h"
#include "cubec/statement_switch.h"
#include "cubec/statement_foreach.h"
#include "cubec/statement_expression.h"
#include "cubec/expression_binary.h"
#include "cubec/literal_identifier.h"
#include "engine/semantic_type.h"
#include "engine/symbol.h"
#include "engine/scope.h"
#include "engine/context.h"
#include "core/node.h"
#include <string.h>

/* Forward declarations */
c_type_t lower_type(allocator_t allocator, semantic_type_t type,
                     const char *module_hash);
c_ir_node_t lower_expr(allocator_t allocator, context_t ctx, node_t node,
                        const char *module_hash);

/**
 * @brief Lower a cubec statement AST node to a C IR statement node.
 */
c_ir_node_t lower_stmt(allocator_t allocator, context_t ctx, node_t node,
                        const char *module_hash) {
  if (!node) return NULL;

  /* Error nodes — skip */
  if (node->kind == CUBEC_NODE_ERROR || node->kind == CUBEC_NODE_STATEMENT_ERROR) {
    return NULL;
  }

  location_t loc = node->location;

  switch (node->kind) {

  /* ===== Block ===== */
  case CUBEC_NODE_STATEMENT_BLOCK: {
    cubec_statement_block_t n = (cubec_statement_block_t)node;
    vec_t stmts = allocator_create(allocator, &g_vec_type,
                                    &(vec_init_t){.auto_dispose = false});
    size_t count = n->statements ? vec_get_size(n->statements) : 0;
    for (size_t i = 0; i < count; i++) {
      node_t child = vec_get(n->statements, i);
      c_ir_node_t c_child = lower_stmt(allocator, ctx, child, module_hash);
      if (c_child) vec_push(stmts, c_child);
    }
    return (c_ir_node_t)c_ir_stmt_block_create(allocator, stmts, loc);
  }

  /* ===== Expression statement ===== */
  case CUBEC_NODE_STATEMENT_EXPRESSION: {
    /* statement_expression wraps a single expression */
    /* The expression is stored as a child — need to find it */
    /* In cubec, STATEMENT_EXPRESSION has a node_t expression field */
    /* But we need the header to know the struct. Let's use a general approach. */
    /* The expression is typically the first child after super */
    /* TODO: read the header for exact struct layout */
    return NULL;
  }

  /* ===== Return ===== */
  case CUBEC_NODE_STATEMENT_RETURN: {
    cubec_statement_return_t n = (cubec_statement_return_t)node;
    c_ir_node_t value = n->expression
        ? lower_expr(allocator, ctx, n->expression, module_hash)
        : NULL;
    return (c_ir_node_t)c_ir_stmt_return_create(allocator, value, loc);
  }

  /* ===== If ===== */
  case CUBEC_NODE_STATEMENT_IF: {
    cubec_statement_if_t n = (cubec_statement_if_t)node;
    c_ir_node_t cond = lower_expr(allocator, ctx, n->condition, module_hash);
    c_ir_node_t then_b = lower_stmt(allocator, ctx, n->then_branch, module_hash);
    c_ir_node_t else_b = n->else_branch
        ? lower_stmt(allocator, ctx, n->else_branch, module_hash)
        : NULL;
    return (c_ir_node_t)c_ir_stmt_if_create(allocator, cond, then_b, else_b, loc);
  }

  /* ===== While ===== */
  case CUBEC_NODE_STATEMENT_WHILE: {
    cubec_statement_while_t n = (cubec_statement_while_t)node;
    c_ir_node_t cond = lower_expr(allocator, ctx, n->condition, module_hash);
    c_ir_node_t body = lower_stmt(allocator, ctx, n->body, module_hash);
    return (c_ir_node_t)c_ir_stmt_while_create(allocator, cond, body, loc);
  }

  /* ===== Do-while ===== */
  case CUBEC_NODE_STATEMENT_DO_WHILE: {
    cubec_statement_do_while_t n = (cubec_statement_do_while_t)node;
    c_ir_node_t body = lower_stmt(allocator, ctx, n->body, module_hash);
    c_ir_node_t cond = lower_expr(allocator, ctx, n->condition, module_hash);
    return (c_ir_node_t)c_ir_stmt_do_while_create(allocator, body, cond, loc);
  }

  /* ===== For ===== */
  case CUBEC_NODE_STATEMENT_FOR: {
    cubec_statement_for_t n = (cubec_statement_for_t)node;
    c_ir_node_t init = n->init ? lower_stmt(allocator, ctx, n->init, module_hash) : NULL;
    c_ir_node_t cond = n->condition ? lower_expr(allocator, ctx, n->condition, module_hash) : NULL;
    c_ir_node_t update = n->increment ? lower_expr(allocator, ctx, n->increment, module_hash) : NULL;
    c_ir_node_t body = lower_stmt(allocator, ctx, n->body, module_hash);
    return (c_ir_node_t)c_ir_stmt_for_create(allocator, init, cond, update, body, loc);
  }

  /* ===== Break / Continue ===== */
  case CUBEC_NODE_STATEMENT_BREAK:
    return (c_ir_node_t)c_ir_stmt_break_create(allocator, loc);

  case CUBEC_NODE_STATEMENT_CONTINUE:
    return (c_ir_node_t)c_ir_stmt_continue_create(allocator, loc);

  /* ===== Declaration ===== */
  case CUBEC_NODE_STATEMENT_DECLARATION: {
    cubec_statement_declaration_t n = (cubec_statement_declaration_t)node;
    /* Comptime/builtin declarations produce no C code */
    if (n->is_comptime || n->is_builtin) return NULL;
    /* The actual variable is in n->declarator */
    /* TODO: resolve variable declaration — for now, skip */
    return NULL;
  }

  /* ===== Function ===== */
  case CUBEC_NODE_STATEMENT_FUNCTION: {
    cubec_statement_function_t n = (cubec_statement_function_t)node;
    /* Comptime/builtin/extern functions — skip or handle specially */
    if (n->is_comptime || n->is_builtin) return NULL;
    if (n->is_extern) {
      /* Extern → just a declaration, no body */
      /* TODO: generate function_decl */
      return NULL;
    }
    /* Regular function — generate function_def */
    /* TODO: full function lowering — resolve name, params, return type, body */
    return NULL;
  }

  /* ===== Struct ===== */
  case CUBEC_NODE_STATEMENT_STRUCT: {
    /* Structs generate typedefs — handled in lower_program */
    return NULL;
  }

  /* ===== Enum ===== */
  case CUBEC_NODE_STATEMENT_ENUM: {
    /* Enums generate enum defs — handled in lower_program */
    return NULL;
  }

  /* ===== Union / CUnion ===== */
  case CUBEC_NODE_STATEMENT_UNION:
  case CUBEC_NODE_STATEMENT_CUNION: {
    return NULL;
  }

  /* ===== Defer ===== */
  case CUBEC_NODE_STATEMENT_DEFER: {
    /* TODO: defer lowering — generate cleanup function + chain registration */
    return NULL;
  }

  /* ===== Switch ===== */
  case CUBEC_NODE_STATEMENT_SWITCH: {
    /* TODO: switch → if-else chain */
    return NULL;
  }

  /* ===== Foreach ===== */
  case CUBEC_NODE_STATEMENT_FOREACH: {
    /* TODO: foreach → while + next call loop */
    return NULL;
  }

  /* ===== Import / Comptime / Test — no C output ===== */
  case CUBEC_NODE_STATEMENT_IMPORT:
  case CUBEC_NODE_STATEMENT_COMPTIME_IF:
  case CUBEC_NODE_STATEMENT_COMPTIME_FOREACH:
  case CUBEC_NODE_STATEMENT_TEST:
  case CUBEC_NODE_STATEMENT_EXPORT_FROM:
  case CUBEC_NODE_STATEMENT_INTERFACE:
  case CUBEC_NODE_STATEMENT_EMPTY:
  case CUBEC_NODE_STATEMENT_DECLARATION_TYPE:
    return NULL;

  default:
    return NULL;
  }
}
