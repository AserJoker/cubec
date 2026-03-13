#include "runtime/error.h"
#include "engine/context.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

cubec_value_t cubec_run_error(cubec_context_t ctx, cubec_vm_t vm,
                              cubec_ast_error_t node) {
  char msg[strlen(node->message) + 128];
  sprintf(msg, "%s at\n  %s:%" PRIuPTR ":%" PRIuPTR, node->message,
          ctx->module->filename, node->super.loc.end.line,
          node->super.loc.end.column);
  return cubec_context_create_error(ctx, msg, NULL);
}