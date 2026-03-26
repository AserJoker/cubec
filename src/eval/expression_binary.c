#include "eval/expression_binary.h"
#include "ast/expression_binary.h"
#include "ast/expression_compute_member.h"
#include "ast/expression_member.h"
#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
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

static cubec_value_t cubec_eval_member_self_opt(
    cubec_context_t ctx, cubec_ast_expression_member_t member,
    const char *filename, cubec_ast_node_t opt, bool prefix) {
  cubec_value_t host = cubec_eval_expression(ctx, member->host, filename);
  if (host->type->kind == CUBEC_TYPE_KIND_ERROR) {
    return host;
  }
  char *field = cubec_location_get(member->field->loc, ctx->allocator);
  cubec_value_t value = cubec_context_get_field(ctx, host, field);
  if (value->type->kind == CUBEC_TYPE_KIND_ERROR) {
    cubec_allocator_free(ctx->allocator, field);
    return cubec_context_convert_compile_error(ctx, &member->super, filename,
                                               value);
  }
  cubec_value_t err = NULL;
  cubec_value_t result = NULL;
  if (!prefix) {
    result =
        cubec_context_create_value(ctx, value->type, true, value->data, NULL);
  }
  if (cubec_location_is(opt->loc, "++")) {
    err = cubec_context_inc_value(ctx, value);
  } else {
    err = cubec_context_dec_value(ctx, value);
  }
  if (err->type->kind == CUBEC_TYPE_KIND_ERROR) {
    cubec_allocator_free(ctx->allocator, field);
    return cubec_context_convert_compile_error(ctx, &member->super, filename,
                                               err);
  }
  if (prefix) {
    result = value;
  }
  err = cubec_context_set_field(ctx, host, field, value);
  if (err->type->kind == CUBEC_TYPE_KIND_ERROR) {
    cubec_allocator_free(ctx->allocator, field);
    return cubec_context_convert_compile_error(ctx, &member->super, filename,
                                               err);
  }
  cubec_allocator_free(ctx->allocator, field);
  return result;
}

static cubec_value_t cubec_eval_compute_member_self_opt(
    cubec_context_t ctx, cubec_ast_expression_compute_member_t member,
    const char *filename, cubec_ast_node_t opt, bool prefix) {
  cubec_value_t host = cubec_eval_expression(ctx, member->host, filename);
  if (host->type->kind == CUBEC_TYPE_KIND_ERROR) {
    return host;
  }
  cubec_value_t vfield = cubec_eval_expression(ctx, member->field, filename);
  if (vfield->type->kind == CUBEC_TYPE_KIND_ERROR) {
    return vfield;
  }
  if (vfield->type->kind == CUBEC_TYPE_KIND_STR) {
    const char *field = *(const char **)vfield->data;
    cubec_value_t value = cubec_context_get_field(ctx, host, field);
    if (value->type->kind == CUBEC_TYPE_KIND_ERROR) {
      return cubec_context_convert_compile_error(ctx, &member->super, filename,
                                                 value);
    }
    cubec_value_t err = NULL;
    cubec_value_t result = NULL;
    if (!prefix) {
      result =
          cubec_context_create_value(ctx, value->type, true, value->data, NULL);
    }
    if (cubec_location_is(opt->loc, "++")) {
      err = cubec_context_inc_value(ctx, value);
    } else {
      err = cubec_context_dec_value(ctx, value);
    }
    if (err->type->kind == CUBEC_TYPE_KIND_ERROR) {
      return cubec_context_convert_compile_error(ctx, &member->super, filename,
                                                 err);
    }
    if (prefix) {
      result = value;
    }
    err = cubec_context_set_field(ctx, host, field, value);
    if (err->type->kind == CUBEC_TYPE_KIND_ERROR) {
      return cubec_context_convert_compile_error(ctx, &member->super, filename,
                                                 err);
    }
    return result;
  } else if (vfield->type->kind >= CUBEC_TYPE_KIND_INT8 &&
             vfield->type->kind <= CUBEC_TYPE_KIND_UINT64) {
    size_t idx = 0;
    if (vfield->type->kind >= CUBEC_TYPE_KIND_INT8 &&
        vfield->type->kind <= CUBEC_TYPE_KIND_INT64) {
      int64_t i = cubec_context_value_to_int64(ctx, vfield);
      if (i < 0) {
        return cubec_context_create_compile_error(ctx, member->field, filename,
                                                  "Invalid subscript");
      }
      idx = i;
    } else if (vfield->type->kind >= CUBEC_TYPE_KIND_UINT8 &&
               vfield->type->kind <= CUBEC_TYPE_KIND_UINT64) {
      idx = cubec_context_value_to_uint64(ctx, vfield);
    }
    cubec_value_t value = cubec_context_get_index(ctx, host, idx);
    if (value->type->kind == CUBEC_TYPE_KIND_ERROR) {
      return cubec_context_convert_compile_error(ctx, &member->super, filename,
                                                 value);
    }
    cubec_value_t err = NULL;
    cubec_value_t result = NULL;
    if (!prefix) {
      result =
          cubec_context_create_value(ctx, value->type, true, value->data, NULL);
    }
    if (cubec_location_is(opt->loc, "++")) {
      err = cubec_context_inc_value(ctx, value);
    } else {
      err = cubec_context_dec_value(ctx, value);
    }
    if (err->type->kind == CUBEC_TYPE_KIND_ERROR) {
      return cubec_context_convert_compile_error(ctx, &member->super, filename,
                                                 err);
    }
    if (prefix) {
      result = value;
    }
    err = cubec_context_set_index(ctx, host, idx, value);
    if (err->type->kind == CUBEC_TYPE_KIND_ERROR) {
      return cubec_context_convert_compile_error(ctx, &member->super, filename,
                                                 err);
    }
    return result;
  } else {
    return cubec_context_create_compile_error(ctx, member->field, filename,
                                              "Invalid subscript");
  }
}

static cubec_value_t cubec_eval_reference_self_opt(
    cubec_context_t ctx, cubec_ast_expression_binary_t ref,
    const char *filename, cubec_ast_node_t opt, bool prefix) {
  cubec_value_t ptr = cubec_eval_expression(ctx, ref->right, filename);
  if (ptr->type->kind == CUBEC_TYPE_KIND_ERROR) {
    return ptr;
  }
  cubec_value_t value = cubec_context_read_ptr(ctx, ptr);
  if (value->type->kind == CUBEC_TYPE_KIND_ERROR) {
    return cubec_context_create_compile_error(ctx, &ref->super, filename,
                                              *(const char **)value->data);
  }
  cubec_value_t err = NULL;
  cubec_value_t result = NULL;
  if (!prefix) {
    result =
        cubec_context_create_value(ctx, value->type, true, value->data, NULL);
  }
  if (cubec_location_is(opt->loc, "++")) {
    err = cubec_context_inc_value(ctx, value);
  } else {
    err = cubec_context_dec_value(ctx, value);
  }
  if (err->type->kind == CUBEC_TYPE_KIND_ERROR) {
    return cubec_context_create_compile_error(ctx, &ref->super, filename,
                                              *(const char **)err->data);
  }
  if (prefix) {
    result = value;
  }
  err = cubec_context_write_ptr(ctx, ptr, value);
  if (err->type->kind == CUBEC_TYPE_KIND_ERROR) {
    return cubec_context_create_compile_error(ctx, &ref->super, filename,
                                              *(const char **)err->data);
  }
  return result;
}

static cubec_value_t cubec_eval_self_opt(cubec_context_t ctx,
                                         cubec_ast_expression_binary_t expr,
                                         const char *filename,
                                         cubec_ast_node_t opt, bool prefix) {
  cubec_ast_node_t node = expr->right;
  if (!prefix) {
    node = expr->left;
  }
  if (node->type == CUBEC_NODE_TYPE_LITERAL_IDENTIFIER) {
    cubec_value_t value = cubec_eval_literal_identifier(
        ctx, (cubec_ast_literal_identifier_t)node, filename);
    if (value->type->kind == CUBEC_TYPE_KIND_ERROR) {
      return value;
    }
    cubec_value_t err = NULL;
    cubec_value_t result = NULL;
    if (!prefix) {
      result =
          cubec_context_create_value(ctx, value->type, true, value->data, NULL);
    }
    if (cubec_location_is(opt->loc, "++")) {
      err = cubec_context_inc_value(ctx, value);
    } else {
      err = cubec_context_dec_value(ctx, value);
    }
    if (err->type->kind == CUBEC_TYPE_KIND_ERROR) {
      return cubec_context_create_compile_error(ctx, &expr->super, filename,
                                                *(const char **)err->data);
    }
    if (prefix) {
      result = value;
    }
    return result;
  } else if (node->type == CUBEC_NODE_TYPE_EXPRESSION_MEMBER) {
    return cubec_eval_member_self_opt(ctx, (cubec_ast_expression_member_t)node,
                                      filename, opt, prefix);
  } else if (node->type == CUBEC_NODE_TYPE_EXPRESSION_COMPUTE_MEMBER) {
    return cubec_eval_compute_member_self_opt(
        ctx, (cubec_ast_expression_compute_member_t)node, filename, opt,
        prefix);
  } else if (cubec_is_reference(node)) {
    return cubec_eval_reference_self_opt(
        ctx, (cubec_ast_expression_binary_t)node, filename, opt, prefix);
  } else {
    return cubec_context_create_compile_error(ctx, &expr->super, filename,
                                              "Expression is not assignable");
  }
}

cubec_value_t cubec_eval_expression_binary(cubec_context_t ctx,
                                           cubec_ast_expression_binary_t expr,
                                           const char *filename) {
  if (!expr->left) {
    if (cubec_location_is(expr->opt->loc, "++")) {
      return cubec_eval_self_opt(ctx, (cubec_ast_expression_binary_t)expr,
                                 filename, expr->opt, true);
    } else if (cubec_location_is(expr->opt->loc, "--")) {
      return cubec_eval_self_opt(ctx, (cubec_ast_expression_binary_t)expr,
                                 filename, expr->opt, true);
    } else {
      cubec_value_t value = cubec_eval_expression(ctx, expr->right, filename);
      if (value->type->kind == CUBEC_TYPE_KIND_ERROR) {
        return value;
      }
      if (cubec_location_is(expr->opt->loc, "+")) {
        value = cubec_context_plus(ctx, value);
      } else if (cubec_location_is(expr->opt->loc, "-")) {
        value = cubec_context_negtive(ctx, value);
      } else if (cubec_location_is(expr->opt->loc, "!")) {
        value = cubec_context_logical_not(ctx, value);
      } else if (cubec_location_is(expr->opt->loc, "~")) {
        value = cubec_context_bitwise_not(ctx, value);
      } else if (cubec_location_is(expr->opt->loc, "&")) {
        value = cubec_context_create_ptr(ctx, value, true, NULL);
      } else if (cubec_location_is(expr->opt->loc, "*")) {
        if (value->type->kind == CUBEC_TYPE_KIND_TYPE) {
          cubec_type_t type = *(cubec_type_t *)value->data;
          type = cubec_context_create_ptr_type(ctx, type, true, false);
          value = cubec_context_create_type_value(ctx, type, false, NULL);
        } else {
          value = cubec_context_read_ptr(ctx, value);
        }
      } else {
        value = cubec_context_create_compile_error(ctx, &expr->super, filename,
                                                   "Invalid operator");
      }
      if (value->type->kind == CUBEC_TYPE_KIND_ERROR) {
        value = cubec_context_convert_compile_error(ctx, &expr->super, filename,
                                                    value);
      }
      return value;
    }
  } else if (!expr->right) {
    if (cubec_location_is(expr->opt->loc, "++")) {
      return cubec_eval_self_opt(ctx, (cubec_ast_expression_binary_t)expr,
                                 filename, expr->opt, false);
    } else if (cubec_location_is(expr->opt->loc, "--")) {
      return cubec_eval_self_opt(ctx, (cubec_ast_expression_binary_t)expr,
                                 filename, expr->opt, false);
    } else {
      return cubec_context_create_compile_error(ctx, &expr->super, filename,
                                                "Invalid operator");
    }
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