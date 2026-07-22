#ifndef _H_CUBEC_ENGINE_COMPTIME_EVAL_
#define _H_CUBEC_ENGINE_COMPTIME_EVAL_
#include "core/allocator.h"
#include "engine/checker.h"
#include "engine/comptime_alloc.h"
#include "engine/comptime_value.h"
#ifdef __cplusplus
extern "C" {
#endif

/* ===== comptime environment (variable bindings) ===== */

struct comptime_env {
  allocator_t allocator;
  comptime_env_t parent;
  strmap_t bindings;   /**< name -> uint64_t addr (not auto-disposed; values in alloc) */
  vec_t temporaries;   /**< temporary comptime_value_t (auto-disposed) */
};

comptime_env_t comptime_env_create(allocator_t allocator, comptime_env_t parent);
void comptime_env_dispose(comptime_env_t self);

/* Low-level addr API: store/retrieve alloc addresses directly */
void comptime_env_bind(comptime_env_t self, const char *name, uint64_t addr);
uint64_t comptime_env_lookup_addr(comptime_env_t self, const char *name);
bool comptime_env_update_addr(comptime_env_t self, const char *name,
                              uint64_t addr);

/* Convenience value API: allocate/read/write through valloc */
void comptime_env_bind_value(comptime_env_t self, comptime_allocator_t valloc,
                              const char *name, comptime_value_t value);
comptime_value_t comptime_env_lookup_value(comptime_env_t self,
                                           comptime_allocator_t valloc,
                                           const char *name);
bool comptime_env_update_value(comptime_env_t self, comptime_allocator_t valloc,
                                const char *name, comptime_value_t value);

comptime_value_t comptime_env_track_temp(comptime_env_t self, comptime_value_t value);

/* ===== control flow signals ===== */

enum comptime_signal_kind {
  COMPTIME_SIGNAL_NONE,
  COMPTIME_SIGNAL_RETURN,
  COMPTIME_SIGNAL_BREAK,
  COMPTIME_SIGNAL_CONTINUE,
  COMPTIME_SIGNAL_ERROR,
  COMPTIME_SIGNAL_FATAL,   /**< panic: unrecoverable, abort compilation */
};

struct comptime_signal {
  enum comptime_signal_kind kind;
  comptime_value_t return_value;
};

typedef struct comptime_signal comptime_signal_t;

/* ===== comptime evaluator ===== */

#define COMPTIME_MAX_LOOP_ITERATIONS 1024
#define COMPTIME_MAX_CALL_STACK_DEPTH 256

struct comptime_eval {
  allocator_t allocator;
  comptime_allocator_t valloc;
  comptime_env_t global_env;
  comptime_env_t current_env;
  int call_depth;
  int loop_depth;
  vec_t cleanup_stack; /**< stack of cleanup_entry to execute on scope exit */
  vec_t captured_envs; /**< captured envs created for function values (disposed at eval teardown) */
  vec_t return_type_stack; /**< stack of semantic_type_t for current function return types */
  bool propagated_return; /**< set by .? when ofError propagation succeeds */
  comptime_value_t propagated_return_value; /**< the ofError result to return */
  bool in_test_block;     /**< true while evaluating a test block body */
};

typedef struct comptime_eval *comptime_eval_t;

comptime_eval_t comptime_eval_create(allocator_t allocator);
void comptime_eval_dispose(comptime_eval_t self);

/* ===== expression evaluation ===== */

comptime_value_t comptime_eval_expr(comptime_eval_t eval, checker_t ctx,
                                     node_t expr);

/* ===== statement execution ===== */

struct comptime_signal comptime_eval_exec_block(comptime_eval_t eval,
                                                 checker_t ctx, node_t block);
struct comptime_signal comptime_eval_exec_stmt(comptime_eval_t eval,
                                                checker_t ctx, node_t stmt);
struct comptime_signal comptime_eval_exec_comptime_if(comptime_eval_t eval,
                                                       checker_t ctx,
                                                       node_t node);
struct comptime_signal comptime_eval_exec_comptime_for(comptime_eval_t eval,
                                                        checker_t ctx,
                                                        node_t node);

#ifdef __cplusplus
}
#endif
#endif
