#include "eval/expression_binary.h"
#include "ast/expression_binary.h"
#include "ast/expression_compute_member.h"
#include "ast/expression_member.h"
#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/string.h"
#include "engine/context.h"
#include "engine/type.h"
#include "engine/value.h"
#include "eval/expression.h"
#include "eval/literal_identifier.h"
#include <inttypes.h>
#include <stdbool.h>
static bool cubec_is_reference(cubec_ast_node_t node) {
  if (node->type == CUBEC_NODE_TYPE_EXPRESSION_BINARY) {
    cubec_ast_expression_binary_t binary = (cubec_ast_expression_binary_t)node;
    if (!binary->left && binary->right &&
        cubec_location_is(binary->opt->loc, "*")) {
      return true;
    }
  }
  return false;
}
static cubec_value_t
cubec_eval_prefix_member_inc(cubec_context_t ctx,
                             cubec_ast_expression_member_t member,
                             const char *filename) {}
static cubec_value_t cubec_eval_prefix_compute_member_inc(
    cubec_context_t ctx, cubec_ast_expression_compute_member_t member,
    const char *filename) {}

static cubec_value_t
cubec_eval_prefix_reference_inc(cubec_context_t ctx,
                                cubec_ast_expression_binary_t ref,
                                const char *filename) {}

static cubec_value_t cubec_eval_prefix_inc(cubec_context_t ctx,
                                           cubec_ast_expression_binary_t expr,
                                           const char *filename) {
  cubec_ast_node_t node = expr->right;
  if (node->type == CUBEC_NODE_TYPE_LITERAL_IDENTIFIER) {
    cubec_value_t value = cubec_eval_literal_identifier(
        ctx, (cubec_ast_literal_identifier_t)node, filename);
    if (value->type->kind == CUBEC_TYPE_KIND_ERROR) {
      return value;
    }
    cubec_value_t err = cubec_context_inc_value(ctx, value);
    if (err->type->kind == CUBEC_TYPE_KIND_ERROR) {
      return cubec_context_create_compile_error(ctx, &expr->super, filename,
                                                *(const char **)err->data);
    }
    return value;
  } else if (node->type == CUBEC_NODE_TYPE_EXPRESSION_MEMBER) {
    return cubec_eval_prefix_member_inc(
        ctx, (cubec_ast_expression_member_t)node, filename);
  } else if (node->type == CUBEC_NODE_TYPE_EXPRESSION_COMPUTE_MEMBER) {
    return cubec_eval_prefix_compute_member_inc(
        ctx, (cubec_ast_expression_compute_member_t)node, filename);
  } else if (cubec_is_reference(node)) {
    return cubec_eval_prefix_reference_inc(
        ctx, (cubec_ast_expression_binary_t)node, filename);
  } else {
    return cubec_context_create_compile_error(ctx, &expr->super, filename,
                                              "Expression is not assignable");
  }
}
static cubec_value_t cubec_eval_prefix_dec(cubec_context_t ctx,
                                           cubec_ast_expression_binary_t expr,
                                           const char *filename) {}
static cubec_value_t cubec_eval_postfix_inc(cubec_context_t ctx,
                                            cubec_ast_expression_binary_t expr,
                                            const char *filename) {}
static cubec_value_t cubec_eval_postfix_dec(cubec_context_t ctx,
                                            cubec_ast_expression_binary_t expr,
                                            const char *filename) {}

cubec_value_t cubec_eval_expression_binary(cubec_context_t ctx,
                                           cubec_ast_expression_binary_t expr,
                                           const char *filename) {
  if (!expr->left) {
    if (cubec_location_is(expr->opt->loc, "++") ||
        cubec_location_is(expr->opt->loc, "--")) {
      cubec_value_t value = NULL;
      cubec_value_t host = NULL;
      char *field = NULL;
      size_t idx = 0;
      if (expr->right->type == CUBEC_NODE_TYPE_LITERAL_IDENTIFIER) {
        value = cubec_eval_literal_identifier(
            ctx, (cubec_ast_literal_identifier_t)expr->right, filename);
        if (value->type->kind == CUBEC_TYPE_KIND_ERROR) {
          return value;
        }
      } else if (expr->right->type == CUBEC_NODE_TYPE_EXPRESSION_BINARY &&
                 !((cubec_ast_expression_binary_t)expr->right)->left &&
                 cubec_location_is(
                     ((cubec_ast_expression_binary_t)expr->right)->opt->loc,
                     "*")) {
        // TODO: ref
      } else if (expr->right->type == CUBEC_NODE_TYPE_EXPRESSION_MEMBER) {
        cubec_ast_expression_member_t member =
            (cubec_ast_expression_member_t)expr->right;
        host = cubec_eval_expression(ctx, member->host, filename);
        if (host->type->kind == CUBEC_TYPE_KIND_ERROR) {
          return host;
        }
        field = cubec_location_get(member->field->loc, ctx->allocator);
        value = cubec_context_get_field(ctx, host, field);
        if (value->type->kind == CUBEC_TYPE_KIND_ERROR) {
          cubec_allocator_free(ctx->allocator, field);
          return value;
        }
      } else if (expr->right->type ==
                 CUBEC_NODE_TYPE_EXPRESSION_COMPUTE_MEMBER) {
        cubec_ast_expression_compute_member_t member =
            (cubec_ast_expression_compute_member_t)expr->right;
        cubec_value_t host = cubec_eval_expression(ctx, member->host, filename);
        if (host->type->kind == CUBEC_TYPE_KIND_ERROR) {
          return host;
        }
        cubec_value_t vfield =
            cubec_eval_expression(ctx, member->field, filename);
        if (vfield->type->kind == CUBEC_TYPE_KIND_ERROR) {
          return vfield;
        }
        if (vfield->type->kind == CUBEC_TYPE_KIND_STR) {
          field = cubec_create_cstring(ctx->allocator,
                                       *(const char **)vfield->data);
        } else if (vfield->type->kind >= CUBEC_TYPE_KIND_INT8 &&
                   vfield->type->kind <= CUBEC_TYPE_KIND_INT64) {
          int64_t i = cubec_context_value_to_int64(ctx, vfield);
          if (i < 0) {
            return cubec_context_create_compile_error(
                ctx, member->field, filename,
                "Index %" PRIiPTR " is before the beginning of the array", i);
          }
          idx = i;
        } else if (vfield->type->kind >= CUBEC_TYPE_KIND_UINT8 &&
                   vfield->type->kind <= CUBEC_TYPE_KIND_UINT64) {
          idx = cubec_context_value_to_uint64(ctx, vfield);
        } else {
          return cubec_context_create_compile_error(
              ctx, member->field, filename, "Invalid subscript");
        }
        if (field) {
          value = cubec_context_get_field(ctx, host, field);
        } else {
          value = cubec_context_get_index(ctx, host, idx);
        }
        if (value->type->kind == CUBEC_TYPE_KIND_ERROR) {
          const char *msg = *(const char **)value->data;
          cubec_allocator_free(ctx->allocator, field);
          return cubec_context_create_compile_error(ctx, &member->super,
                                                    filename, msg);
        }
      } else {
        return cubec_context_create_compile_error(
            ctx, &expr->super, filename, "Expression is not assignable");
      }
      cubec_value_t res = ctx->value_undefined;
      if (cubec_location_is(expr->opt->loc, "++")) {
        res = cubec_context_inc_value(ctx, value);
      } else if (cubec_location_is(expr->opt->loc, "--")) {
        res = cubec_context_dec_value(ctx, value);
      } else {
        res = cubec_context_create_error(ctx, "Unknown postfix operator ");
      }
      if (res->type->kind == CUBEC_TYPE_KIND_ERROR) {
        const char *msg = *(const char **)res->data;
        cubec_allocator_free(ctx->allocator, field);
        return cubec_context_create_compile_error(ctx, &expr->super, filename,
                                                  msg);
      }
      res = value;
      if (expr->right->type == CUBEC_NODE_TYPE_EXPRESSION_MEMBER) {
        cubec_value_t err = cubec_context_set_field(ctx, host, field, value);
        if (err->type->kind == CUBEC_TYPE_KIND_ERROR) {
          const char *msg = *(const char **)err->data;
          cubec_allocator_free(ctx->allocator, field);
          return cubec_context_create_compile_error(ctx, &expr->super, filename,
                                                    msg);
        }
      } else if (expr->right->type ==
                 CUBEC_NODE_TYPE_EXPRESSION_COMPUTE_MEMBER) {
        if (field) {
          cubec_value_t err = cubec_context_set_field(ctx, host, field, value);
          if (err->type->kind == CUBEC_TYPE_KIND_ERROR) {
            const char *msg = *(const char **)err->data;
            cubec_allocator_free(ctx->allocator, field);
            return cubec_context_create_compile_error(ctx, &expr->super,
                                                      filename, msg);
          }
        } else {
          cubec_value_t err = cubec_context_set_index(ctx, host, idx, value);
          if (err->type->kind == CUBEC_TYPE_KIND_ERROR) {
            const char *msg = *(const char **)err->data;
            return cubec_context_create_compile_error(ctx, &expr->super,
                                                      filename, msg);
          }
        }
      } else if (expr->right->type == CUBEC_NODE_TYPE_EXPRESSION_BINARY &&
                 !((cubec_ast_expression_binary_t)expr->right)->left &&
                 cubec_location_is(
                     ((cubec_ast_expression_binary_t)expr->right)->opt->loc,
                     "*")) {
        // TODO: ref
      }
      cubec_allocator_free(ctx->allocator, field);
      return res;
    } else {
    }
  } else if (!expr->right) {
    cubec_value_t value = NULL;
    cubec_value_t host = NULL;
    char *field = NULL;
    size_t idx = 0;
    if (expr->left->type == CUBEC_NODE_TYPE_LITERAL_IDENTIFIER) {
      value = cubec_eval_literal_identifier(
          ctx, (cubec_ast_literal_identifier_t)expr->left, filename);
      if (value->type->kind == CUBEC_TYPE_KIND_ERROR) {
        return value;
      }
    } else if (expr->left->type == CUBEC_NODE_TYPE_EXPRESSION_MEMBER) {
      cubec_ast_expression_member_t member =
          (cubec_ast_expression_member_t)expr->left;
      host = cubec_eval_expression(ctx, member->host, filename);
      if (host->type->kind == CUBEC_TYPE_KIND_ERROR) {
        return host;
      }
      field = cubec_location_get(member->field->loc, ctx->allocator);
      value = cubec_context_get_field(ctx, host, field);
      if (value->type->kind == CUBEC_TYPE_KIND_ERROR) {
        cubec_allocator_free(ctx->allocator, field);
        return value;
      }
    } else if (expr->left->type == CUBEC_NODE_TYPE_EXPRESSION_COMPUTE_MEMBER) {
      cubec_ast_expression_compute_member_t member =
          (cubec_ast_expression_compute_member_t)expr->left;
      cubec_value_t host = cubec_eval_expression(ctx, member->host, filename);
      if (host->type->kind == CUBEC_TYPE_KIND_ERROR) {
        return host;
      }
      cubec_value_t vfield =
          cubec_eval_expression(ctx, member->field, filename);
      if (vfield->type->kind == CUBEC_TYPE_KIND_ERROR) {
        return vfield;
      }
      if (vfield->type->kind == CUBEC_TYPE_KIND_STR) {
        field =
            cubec_create_cstring(ctx->allocator, *(const char **)vfield->data);
      } else if (vfield->type->kind >= CUBEC_TYPE_KIND_INT8 &&
                 vfield->type->kind <= CUBEC_TYPE_KIND_INT64) {
        int64_t i = cubec_context_value_to_int64(ctx, vfield);
        if (i < 0) {
          return cubec_context_create_compile_error(
              ctx, member->field, filename,
              "Index %" PRIiPTR " is before the beginning of the array", i);
        }
        idx = i;
      } else if (vfield->type->kind >= CUBEC_TYPE_KIND_UINT8 &&
                 vfield->type->kind <= CUBEC_TYPE_KIND_UINT64) {
        idx = cubec_context_value_to_uint64(ctx, vfield);
      } else {
        return cubec_context_create_compile_error(ctx, member->field, filename,
                                                  "Invalid subscript");
      }
      if (field) {
        value = cubec_context_get_field(ctx, host, field);
      } else {
        value = cubec_context_get_index(ctx, host, idx);
      }
      if (value->type->kind == CUBEC_TYPE_KIND_ERROR) {
        const char *msg = *(const char **)value->data;
        cubec_allocator_free(ctx->allocator, field);
        return cubec_context_create_compile_error(ctx, &member->super, filename,
                                                  msg);
      }
    } else if (expr->left->type == CUBEC_NODE_TYPE_EXPRESSION_BINARY &&
               !((cubec_ast_expression_binary_t)expr->left)->left &&
               cubec_location_is(
                   ((cubec_ast_expression_binary_t)expr->left)->opt->loc,
                   "*")) {
      // TODO: ref
    } else {
      return cubec_context_create_compile_error(ctx, &expr->super, filename,
                                                "Expression is not assignable");
    }
    cubec_value_t res =
        cubec_context_create_value(ctx, value->type, true, value->data, NULL);
    if (cubec_location_is(expr->opt->loc, "++")) {
      cubec_value_t err = cubec_context_inc_value(ctx, value);
      if (err->type->kind == CUBEC_TYPE_KIND_ERROR) {
        res = err;
      }
    } else if (cubec_location_is(expr->opt->loc, "--")) {
      cubec_value_t err = cubec_context_dec_value(ctx, value);
      if (err->type->kind == CUBEC_TYPE_KIND_ERROR) {
        res = err;
      }
    } else {
      res = cubec_context_create_error(ctx, "Unknown postfix operator ");
    }
    if (res->type->kind == CUBEC_TYPE_KIND_ERROR) {
      const char *msg = *(const char **)res->data;
      cubec_allocator_free(ctx->allocator, field);
      return cubec_context_create_compile_error(ctx, &expr->super, filename,
                                                msg);
    }
    if (expr->left->type == CUBEC_NODE_TYPE_EXPRESSION_MEMBER) {
      cubec_value_t err = cubec_context_set_field(ctx, host, field, value);
      if (err->type->kind == CUBEC_TYPE_KIND_ERROR) {
        const char *msg = *(const char **)err->data;
        cubec_allocator_free(ctx->allocator, field);
        return cubec_context_create_compile_error(ctx, &expr->super, filename,
                                                  msg);
      }
    } else if (expr->left->type == CUBEC_NODE_TYPE_EXPRESSION_COMPUTE_MEMBER) {
      if (field) {
        cubec_value_t err = cubec_context_set_field(ctx, host, field, value);
        if (err->type->kind == CUBEC_TYPE_KIND_ERROR) {
          const char *msg = *(const char **)err->data;
          cubec_allocator_free(ctx->allocator, field);
          return cubec_context_create_compile_error(ctx, &expr->super, filename,
                                                    msg);
        }
      } else {
        cubec_value_t err = cubec_context_set_index(ctx, host, idx, value);
        if (err->type->kind == CUBEC_TYPE_KIND_ERROR) {
          const char *msg = *(const char **)err->data;
          return cubec_context_create_compile_error(ctx, &expr->super, filename,
                                                    msg);
        }
      }
    } else if (expr->left->type == CUBEC_NODE_TYPE_EXPRESSION_BINARY &&
               !((cubec_ast_expression_binary_t)expr->left)->left &&
               cubec_location_is(
                   ((cubec_ast_expression_binary_t)expr->left)->opt->loc,
                   "*")) {
      // TODO: ref
    }
    cubec_allocator_free(ctx->allocator, field);
    return res;
  } else {
    cubec_value_t left = cubec_eval_expression(ctx, expr->left, filename);
    if (left->type->kind == CUBEC_TYPE_KIND_ERROR) {
      return left;
    }
    cubec_value_t right = cubec_eval_expression(ctx, expr->right, filename);
    if (right->type->kind == CUBEC_TYPE_KIND_ERROR) {
      return right;
    }
  }
  return cubec_context_create_compile_error(ctx, &expr->super, filename,
                                            "Invalid expression");
}