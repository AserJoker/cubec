#include "eval/expression_member.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/type.h"
#include "engine/value.h"
#include "eval/expression.h"
cubec_value_t cubec_eval_expression_member(cubec_context_t ctx,
                                           cubec_ast_node_t expr,
                                           const char *filename) {
  cubec_ast_node_t host_node = cubec_map_get(expr->children, "host", NULL);
  cubec_ast_node_t field_node = cubec_map_get(expr->children, "field", NULL);
  cubec_value_t host = cubec_eval_expression(ctx, host_node, filename);
  if (host->type->kind == CUBEC_TYPE_KIND_ERROR) {
    return host;
  }
  char *field = cubec_location_get(field_node->loc, ctx->allocator);
  if (host->type->kind == CUBEC_TYPE_KIND_STRUCT ||
      host->type->kind == CUBEC_TYPE_KIND_UNION) {
    cubec_value_t val = cubec_context_get_field(ctx, host, field);
    cubec_allocator_free(ctx->allocator, field);
    if (val->type->kind == CUBEC_TYPE_KIND_ERROR) {
      const char *msg = *(const char **)val->data;
      return cubec_context_create_compile_error(ctx, expr, filename, msg);
    }
    return val;
  } else {
    return cubec_context_create_compile_error(ctx, expr, filename,
                                              "Member reference invalid");
  }
}