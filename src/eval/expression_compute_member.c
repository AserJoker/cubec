#include "eval/expression_compute_member.h"
#include "ast/node.h"
#include "engine/context.h"
#include "engine/type.h"
#include "engine/value.h"
#include "eval/expression.h"
#include <inttypes.h>

cubec_value_t cubec_eval_expression_compute_member(cubec_context_t ctx,
                                                   cubec_ast_node_t expr,
                                                   const char *filename) {
  cubec_ast_node_t host_node = cubec_map_get(expr->children, "host", NULL);
  cubec_ast_node_t field_node = cubec_map_get(expr->children, "field", NULL);
  cubec_value_t host = cubec_eval_expression(ctx, host_node, filename);
  if (!host) {
    return NULL;
  }
  if (host->type->kind == CUBEC_TYPE_KIND_ERROR) {
    return host;
  }
  cubec_value_t field = cubec_eval_expression(ctx, field_node, filename);
  if (!field) {
    return NULL;
  }
  if (field->type->kind == CUBEC_TYPE_KIND_ERROR) {
    return field;
  }
  if (field->type->kind == CUBEC_TYPE_KIND_STR) {
    const char *f = *(const char **)field->data;
    return cubec_context_get_field(ctx, host, f);
  } else if (field->type->kind >= CUBEC_TYPE_KIND_INT8 &&
             field->type->kind <= CUBEC_TYPE_KIND_INT64) {
    int64_t idx = cubec_context_value_to_int64(ctx, field);
    if (idx < 0) {
      return cubec_context_create_compile_error(
          ctx, expr, filename,
          "Index %" PRIiPTR " is before the beginning of the array", idx);
    }
    cubec_value_t res = cubec_context_get_index(ctx, host, idx);
    if (res->type->kind == CUBEC_TYPE_KIND_ERROR) {
      const char *msg = *(const char **)res->data;
      return cubec_context_create_compile_error(ctx, expr, filename, msg);
    }
    return res;
  } else if (field->type->kind >= CUBEC_TYPE_KIND_UINT8 &&
             field->type->kind <= CUBEC_TYPE_KIND_INT64) {
    uint64_t idx = cubec_context_value_to_uint64(ctx, field);
    cubec_value_t res = cubec_context_get_index(ctx, host, idx);
    if (res->type->kind == CUBEC_TYPE_KIND_ERROR) {
      const char *msg = *(const char **)res->data;
      return cubec_context_create_compile_error(ctx, expr, filename, msg);
    }
    return res;
  } else {
    return cubec_context_create_compile_error(ctx, expr, filename,
                                              "Invalid subscript");
  }
}