#ifndef _H_CUBEC_C_IR_
#define _H_CUBEC_C_IR_
#include "core/allocator.h"
#include "core/location.h"
#include "core/string.h"
#include "core/vec.h"
#include "c/c_type.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief C IR node kind enumeration.
 *
 * Each node kind corresponds to a C-level construct.
 * C IR nodes carry source_loc from the original cubec AST for #line generation.
 */
enum c_ir_kind {
  /* Compilation unit */
  C_IR_UNIT,

  /* Top-level declarations */
  C_IR_INCLUDE,
  C_IR_TYPEDEF,
  C_IR_FORWARD_DECL,
  C_IR_FUNCTION_DECL,
  C_IR_FUNCTION_DEF,
  C_IR_VARIABLE_DECL,
  C_IR_ENUM_DEF,

  /* Statements */
  C_IR_STMT_BLOCK,
  C_IR_STMT_EXPR,
  C_IR_STMT_RETURN,
  C_IR_STMT_IF,
  C_IR_STMT_WHILE,
  C_IR_STMT_DO_WHILE,
  C_IR_STMT_FOR,
  C_IR_STMT_BREAK,
  C_IR_STMT_CONTINUE,
  C_IR_STMT_GOTO,
  C_IR_STMT_LABEL,
  C_IR_STMT_LOCAL_DECL,
  C_IR_STMT_STMT_EXPR,

  /* Expressions */
  C_IR_EXPR_BINARY,
  C_IR_EXPR_UNARY,
  C_IR_EXPR_CALL,
  C_IR_EXPR_MEMBER,
  C_IR_EXPR_SUBSCRIPT,
  C_IR_EXPR_CAST,
  C_IR_EXPR_TERNARY,
  C_IR_EXPR_COMPOUND,
  C_IR_EXPR_SIZEOF,
  C_IR_EXPR_ALIGNOF,
  C_IR_EXPR_STRING,
  C_IR_EXPR_NUMERIC,
  C_IR_EXPR_CHAR,
  C_IR_EXPR_IDENT,
  C_IR_EXPR_NULL,
  C_IR_EXPR_BOOL,
  C_IR_EXPR_INITIALIZER,
};

/** @brief Unified C IR node pointer type. Void pointer — cast based on kind. */
typedef void *c_ir_node_t;

/** @brief Common header at the start of every C IR node (kind + source_loc). */
struct _c_ir_header_t {
  enum c_ir_kind kind;
  location_t source_loc;
};

/** @brief Get the kind of any C IR node. */
static inline enum c_ir_kind c_ir_get_kind(c_ir_node_t node) {
  return node ? ((struct _c_ir_header_t *)node)->kind : (enum c_ir_kind)-1;
}

/** @brief Get the source location of any C IR node. */
static inline location_t c_ir_get_source_loc(c_ir_node_t node) {
  return node ? ((struct _c_ir_header_t *)node)->source_loc
              : (location_t){0};
}

/** @brief Recursively dispose a C IR node. Sets *node to NULL. */
void c_ir_dispose(allocator_t allocator, c_ir_node_t *node);

/** @brief Dispose all nodes in a vec_t of c_ir_node_t. */
void c_ir_dispose_vec(allocator_t allocator, vec_t *vec);

#ifdef __cplusplus
}
#endif
#endif
