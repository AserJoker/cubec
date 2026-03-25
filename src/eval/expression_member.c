#include "eval/expression_member.h"
#include "core/allocator.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/type.h"
#include "engine/value.h"
#include "eval/expression.h"
cubec_value_t cubec_eval_expression_member(cubec_context_t ctx,
                                           cubec_ast_expression_member_t expr,
                                           const char *filename) {
  cubec_value_t host = cubec_eval_expression(ctx, expr->host, filename);
  if (host->type->kind == CUBEC_TYPE_KIND_ERROR) {
    return host;
  }
  char *field = cubec_location_get(expr->field->loc, ctx->allocator);
  if (host->type->kind == CUBEC_TYPE_KIND_STRUCT ||
      host->type->kind == CUBEC_TYPE_KIND_UNION) {
    cubec_value_t val = cubec_context_get_field(ctx, host, field);
    cubec_allocator_free(ctx->allocator, field);
    if (val->type->kind == CUBEC_TYPE_KIND_ERROR) {
      const char *msg = *(const char **)val->data;
      return cubec_context_create_compile_error(ctx, &expr->super, filename,
                                                msg);
    }
    return val;
  } else {
    return cubec_context_create_compile_error(ctx, &expr->super, filename,
                                              "Member reference invalid");
  }
}