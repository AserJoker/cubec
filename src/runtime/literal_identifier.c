#include "runtime/literal_identifier.h"
#include "core/allocator.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/value.h"

cubec_value_t
cubec_run_literal_identifier(cubec_context_t ctx, cubec_vm_t vm,
                             cubec_ast_literal_identifier_t node) {
  char *s = cubec_location_get(node->super.loc, ctx->allocator);
  cubec_value_t val = cubec_context_load_value(ctx, s);
  cubec_allocator_free(ctx->allocator, s);
  return val;
}