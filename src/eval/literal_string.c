#include "eval/literal_string.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "core/location.h"
#include "engine/context.h"
#include <stdbool.h>
#include <string.h>

cubec_value_t cubec_eval_literal_string(cubec_context_t ctx,
                                        cubec_ast_node_t str,
                                        const char *filename) {
  char *s = cubec_location_get_str(str->loc, ctx->allocator);
  cubec_value_t res = cubec_context_create_str(ctx, s, false, NULL);
  cubec_allocator_free(ctx->allocator, s);
  return res;
}