#include "run/run.h"
#include "engine/vm.h"
#include "engine/void_type.h"

value_t run_program(context_t ctx, node_t node, bool shadow) {
  (void)ctx;
  (void)node;
  (void)shadow;
  /* TODO: iterate program->statements, run each statement */
  return create_void_value(ctx->vm);
}
