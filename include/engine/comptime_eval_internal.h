#ifndef _H_CUBEC_ENGINE_COMPTIME_EVAL_INTERNAL_
#define _H_CUBEC_ENGINE_COMPTIME_EVAL_INTERNAL_
#include "engine/comptime_eval.h"
#include "engine/checker_type_util.h"
#include "engine/symbol.h"
#include "core/string.h"
#include "cubec/literal_identifier.h"
#include "cubec/statement_function.h"
#include "cubec/function_argument.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Internal helpers shared between comptime_eval_expr.c and comptime_eval_stmt.c */

/* --- cleanup stack entries --- */

enum cleanup_kind { CLEANUP_DEFER, CLEANUP_USING };

struct cleanup_entry {
  enum cleanup_kind kind;
  union {
    node_t defer_body;                                          /* CLEANUP_DEFER */
    struct { const char *name; semantic_type_t type; } using_info; /* CLEANUP_USING */
  };
};

static inline const char *_eval_ident_str(node_t id_node) {
  if (!id_node || id_node->kind != CUBEC_NODE_LITERAL_IDENTIFIER) return NULL;
  return string_get(((cubec_literal_identifier_t)id_node)->value);
}

static inline comptime_value_t _eval_temp(comptime_eval_t eval, comptime_value_t val) {
  if (val && eval->current_env)
    comptime_env_track_temp(eval->current_env, val);
  return val;
}

static inline comptime_value_t _eval_error_val(comptime_eval_t eval) {
  return _eval_temp(eval, comptime_value_create_error(eval->allocator));
}

static inline comptime_signal_t _eval_signal_none(void) {
  return (comptime_signal_t){.kind = COMPTIME_SIGNAL_NONE, .return_value = NULL};
}

static inline comptime_signal_t _eval_signal_return(comptime_value_t val) {
  return (comptime_signal_t){.kind = COMPTIME_SIGNAL_RETURN,
                              .return_value = val};
}

static inline comptime_signal_t _eval_signal_break(void) {
  return (comptime_signal_t){.kind = COMPTIME_SIGNAL_BREAK,
                              .return_value = NULL};
}

static inline comptime_signal_t _eval_signal_continue(void) {
  return (comptime_signal_t){.kind = COMPTIME_SIGNAL_CONTINUE,
                              .return_value = NULL};
}

static inline comptime_signal_t _eval_signal_error(void) {
  return (comptime_signal_t){.kind = COMPTIME_SIGNAL_ERROR,
                              .return_value = NULL};
}

/* Forward declarations for cross-file calls */
comptime_value_t _comptime_eval_expr(comptime_eval_t eval, checker_t ctx,
                                      node_t expr);
comptime_signal_t _comptime_exec_block(comptime_eval_t eval, checker_t ctx,
                                        node_t block);
comptime_signal_t _comptime_exec_stmt(comptime_eval_t eval, checker_t ctx,
                                       node_t stmt);
comptime_value_t _eval_call_function(comptime_eval_t eval, checker_t ctx,
                                      comptime_value_t callee,
                                      comptime_value_t *args, size_t acount,
                                      node_t call_node);

/* Create a comptime function value from a method symbol's ast_node.
 * Used by foreach dispatch and _eval_member for instance method calls. */
comptime_value_t _comptime_create_method_value(comptime_eval_t eval,
                                                checker_t ctx,
                                                struct symbol *method_sym);

#ifdef __cplusplus
}
#endif
#endif
