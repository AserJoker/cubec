#ifndef _H_CUBEC_ENGINE_COMPTIME_EVAL_INTERNAL_
#define _H_CUBEC_ENGINE_COMPTIME_EVAL_INTERNAL_
#include "engine/comptime_eval.h"
#include "engine/checker_type_util.h"
#include "core/string.h"
#include "cubec/literal_identifier.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Internal helpers shared between comptime_eval_expr.c and comptime_eval_stmt.c */

static inline const char *_eval_ident_str(node_t id_node) {
  if (!id_node || id_node->kind != CUBEC_NODE_LITERAL_IDENTIFIER) return NULL;
  return string_get(((cubec_literal_identifier_t)id_node)->value);
}

static inline comptime_value_t _eval_error_val(comptime_eval_t eval) {
  return comptime_value_create_error(eval->allocator);
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

#ifdef __cplusplus
}
#endif
#endif
