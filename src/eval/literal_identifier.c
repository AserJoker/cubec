#include "eval/literal_identifier.h"
#include "ast/node.h"

cubec_value_t cubec_eval_literal_identifier(cubec_context_t ctx,
                                            cubec_ast_node_t identifier,
                                            const char *filename) {
  char *name = cubec_location_get(identifier->loc, ctx->allocator);
  cubec_value_t val = cubec_context_load_value(ctx, name);
  cubec_allocator_free(ctx->allocator, name);
  if (!val) {
    return cubec_context_create_compile_error(ctx, identifier, filename,
                                              "Use of undeclared identifier");
  }
  return val;
}