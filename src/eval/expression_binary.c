#include "eval/expression_binary.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/map.h"
#include "core/position.h"
#include "engine/context.h"
#include "engine/function.h"
#include "engine/result.h"
#include "engine/type.h"
#include "engine/value.h"
#include "eval/expression.h"
#include "eval/literal_identifier.h"
#include <inttypes.h>
#include <stdbool.h>
static bool cubec_is_reference(cubec_ast_node_t node) {
  if (node->type == CUBEC_NODE_TYPE_EXPRESSION_BINARY) {
    cubec_ast_node_t left = cubec_map_get(node->children, "left", NULL);
    cubec_ast_node_t right = cubec_map_get(node->children, "right", NULL);
    cubec_ast_node_t opt = cubec_map_get(node->children, "opt", NULL);
    if (!left && right && cubec_location_is(opt->loc, "*")) {
      return true;
    }
  }
  return false;
}

static cubec_value_t cubec_eval_member_self_opt(cubec_context_t ctx,
                                                cubec_ast_node_t member,
                                                const char *filename,
                                                cubec_ast_node_t opt,
                                                bool prefix) {
  cubec_ast_node_t host_node = cubec_map_get(member->children, "host", NULL);
  cubec_ast_node_t field_node = cubec_map_get(member->children, "field", NULL);
  cubec_value_t host = cubec_eval_expression(ctx, host_node, filename);
  if (host->type->kind == CUBEC_TYPE_KIND_ERROR) {
    return host;
  }
  char *field = cubec_location_get(field_node->loc, ctx->allocator);
  cubec_value_t value = cubec_context_get_field(ctx, host, field);
  if (value->type->kind == CUBEC_TYPE_KIND_ERROR) {
    cubec_allocator_free(ctx->allocator, field);
    return cubec_context_convert_compile_error(ctx, member, filename, value);
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
    return cubec_context_convert_compile_error(ctx, member, filename, err);
  }
  if (prefix) {
    result = value;
  }
  err = cubec_context_set_field(ctx, host, field, value);
  if (err->type->kind == CUBEC_TYPE_KIND_ERROR) {
    cubec_allocator_free(ctx->allocator, field);
    return cubec_context_convert_compile_error(ctx, member, filename, err);
  }
  cubec_allocator_free(ctx->allocator, field);
  return result;
}

static cubec_value_t cubec_eval_compute_member_self_opt(cubec_context_t ctx,
                                                        cubec_ast_node_t member,
                                                        const char *filename,
                                                        cubec_ast_node_t opt,
                                                        bool prefix) {
  cubec_ast_node_t host_node = cubec_map_get(member->children, "host", NULL);
  cubec_ast_node_t field_node = cubec_map_get(member->children, "field", NULL);
  cubec_value_t host = cubec_eval_expression(ctx, host_node, filename);
  if (host->type->kind == CUBEC_TYPE_KIND_ERROR) {
    return host;
  }
  cubec_value_t vfield = cubec_eval_expression(ctx, field_node, filename);
  if (vfield->type->kind == CUBEC_TYPE_KIND_ERROR) {
    return vfield;
  }
  if (vfield->type->kind == CUBEC_TYPE_KIND_STR) {
    const char *field = *(const char **)vfield->data;
    cubec_value_t value = cubec_context_get_field(ctx, host, field);
    if (value->type->kind == CUBEC_TYPE_KIND_ERROR) {
      return cubec_context_convert_compile_error(ctx, member, filename, value);
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
      return cubec_context_convert_compile_error(ctx, member, filename, err);
    }
    if (prefix) {
      result = value;
    }
    err = cubec_context_set_field(ctx, host, field, value);
    if (err->type->kind == CUBEC_TYPE_KIND_ERROR) {
      return cubec_context_convert_compile_error(ctx, member, filename, err);
    }
    return result;
  } else if (vfield->type->kind >= CUBEC_TYPE_KIND_INT8 &&
             vfield->type->kind <= CUBEC_TYPE_KIND_UINT64) {
    size_t idx = 0;
    if (vfield->type->kind >= CUBEC_TYPE_KIND_INT8 &&
        vfield->type->kind <= CUBEC_TYPE_KIND_INT64) {
      int64_t i = cubec_context_value_to_int64(ctx, vfield);
      if (i < 0) {
        return cubec_context_create_compile_error(ctx, field_node, filename,
                                                  "Invalid subscript");
      }
      idx = i;
    } else if (vfield->type->kind >= CUBEC_TYPE_KIND_UINT8 &&
               vfield->type->kind <= CUBEC_TYPE_KIND_UINT64) {
      idx = cubec_context_value_to_uint64(ctx, vfield);
    }
    cubec_value_t value = cubec_context_get_index(ctx, host, idx);
    if (value->type->kind == CUBEC_TYPE_KIND_ERROR) {
      return cubec_context_convert_compile_error(ctx, member, filename, value);
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
      return cubec_context_convert_compile_error(ctx, member, filename, err);
    }
    if (prefix) {
      result = value;
    }
    err = cubec_context_set_index(ctx, host, idx, value);
    if (err->type->kind == CUBEC_TYPE_KIND_ERROR) {
      return cubec_context_convert_compile_error(ctx, member, filename, err);
    }
    return result;
  } else {
    return cubec_context_create_compile_error(ctx, field_node, filename,
                                              "Invalid subscript");
  }
}

static cubec_value_t cubec_eval_reference_self_opt(cubec_context_t ctx,
                                                   cubec_ast_node_t ref,
                                                   const char *filename,
                                                   cubec_ast_node_t opt,
                                                   bool prefix) {
  cubec_ast_node_t right = cubec_map_get(ref->children, "right", NULL);
  cubec_value_t ptr = cubec_eval_expression(ctx, right, filename);
  if (ptr->type->kind == CUBEC_TYPE_KIND_ERROR) {
    return ptr;
  }
  cubec_value_t value = cubec_context_read_ptr(ctx, ptr);
  if (value->type->kind == CUBEC_TYPE_KIND_ERROR) {
    return cubec_context_create_compile_error(ctx, ref, filename,
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
    return cubec_context_create_compile_error(ctx, ref, filename,
                                              *(const char **)err->data);
  }
  if (prefix) {
    result = value;
  }
  err = cubec_context_write_ptr(ctx, ptr, value);
  if (err->type->kind == CUBEC_TYPE_KIND_ERROR) {
    return cubec_context_create_compile_error(ctx, ref, filename,
                                              *(const char **)err->data);
  }
  return result;
}

static cubec_value_t cubec_eval_self_opt(cubec_context_t ctx,
                                         cubec_ast_node_t expr,
                                         const char *filename,
                                         cubec_ast_node_t opt, bool prefix) {
  cubec_ast_node_t node = cubec_map_get(expr->children, "right", NULL);
  if (!prefix) {
    node = cubec_map_get(expr->children, "left", NULL);
  }
  if (node->type == CUBEC_NODE_TYPE_LITERAL_IDENTIFIER) {
    cubec_value_t value = cubec_eval_literal_identifier(ctx, node, filename);
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
      return cubec_context_create_compile_error(ctx, expr, filename,
                                                *(const char **)err->data);
    }
    if (prefix) {
      result = value;
    }
    return result;
  } else if (node->type == CUBEC_NODE_TYPE_EXPRESSION_MEMBER) {
    return cubec_eval_member_self_opt(ctx, node, filename, opt, prefix);
  } else if (node->type == CUBEC_NODE_TYPE_EXPRESSION_COMPUTE_MEMBER) {
    return cubec_eval_compute_member_self_opt(ctx, node, filename, opt, prefix);
  } else if (cubec_is_reference(node)) {
    return cubec_eval_reference_self_opt(ctx, node, filename, opt, prefix);
  } else {
    return cubec_context_create_compile_error(ctx, expr, filename,
                                              "Expression is not assignable");
  }
}

cubec_value_t cubec_eval_expression_binary(cubec_context_t ctx,
                                           cubec_ast_node_t expr,
                                           const char *filename) {
  cubec_ast_node_t left_node = cubec_map_get(expr->children, "left", NULL);
  cubec_ast_node_t right_node = cubec_map_get(expr->children, "right", NULL);
  cubec_ast_node_t opt = cubec_map_get(expr->children, "opt", NULL);
  if (!left_node) {
    if (cubec_location_is(opt->loc, "++")) {
      return cubec_eval_self_opt(ctx, expr, filename, opt, true);
    } else if (cubec_location_is(opt->loc, "--")) {
      return cubec_eval_self_opt(ctx, expr, filename, opt, true);
    } else {
      cubec_value_t value = cubec_eval_expression(ctx, right_node, filename);
      if (value->type->kind == CUBEC_TYPE_KIND_ERROR) {
        return value;
      }
      if (cubec_location_is(opt->loc, "+")) {
        value = cubec_context_plus(ctx, value);
      } else if (cubec_location_is(opt->loc, "-")) {
        value = cubec_context_negtive(ctx, value);
      } else if (cubec_location_is(opt->loc, "!")) {
        value = cubec_context_logical_not(ctx, value);
      } else if (cubec_location_is(opt->loc, "~")) {
        value = cubec_context_bitwise_not(ctx, value);
      } else if (cubec_location_is(opt->loc, "&")) {
        value = cubec_context_create_ptr(ctx, value, true, NULL);
      } else if (cubec_location_is(opt->loc, "*")) {
        if (value->type->kind == CUBEC_TYPE_KIND_TYPE) {
          cubec_type_t type = *(cubec_type_t *)value->data;
          type = cubec_context_create_ptr_type(ctx, type, true, false);
          value = cubec_context_create_type_value(ctx, type, false, NULL);
        } else {
          value = cubec_context_read_ptr(ctx, value);
        }
      } else if (cubec_location_is(opt->loc, "typeof")) {
        value = cubec_context_create_type_value(ctx, value->type, false, NULL);
      } else if (cubec_location_is(opt->loc, "sizeof")) {
        value =
            cubec_context_create_uint64(ctx, value->type->size, false, NULL);
      } else if (cubec_location_is(opt->loc, "try")) {
        if (!ctx->scope_value ||
            ctx->scope_value->type->kind != CUBEC_TYPE_KIND_FUNCTION) {
          value = cubec_context_create_error(
              ctx, "try expression only used in function");
        } else {
          if (value->type->kind == CUBEC_TYPE_KIND_RESULT) {
            cubec_result_data_t data = value->data;
            cubec_result_meta_t meta = value->type->meta;
            if (data->flag) {
              cubec_value_t func = ctx->scope_value;
              cubec_function_meta_t func_meta = func->data;
              if (func_meta->type->kind == CUBEC_TYPE_KIND_RESULT) {
                value = cubec_context_create_error(
                    ctx, "try expression only used in Result function");
              } else {
                value = cubec_context_create_value(ctx, meta->error_type, false,
                                                   &data->data, NULL);
                value = cubec_context_create_result(ctx, func_meta->type, NULL,
                                                    value, false, NULL);
              }
            } else {
              value = cubec_context_create_value(ctx, meta->type, false,
                                                 &data->data, NULL);
            }
          }
        }
      } else {
        value = cubec_context_create_compile_error(ctx, expr, filename,
                                                   "Invalid operator");
      }
      if (value->type->kind == CUBEC_TYPE_KIND_ERROR) {
        value = cubec_context_convert_compile_error(ctx, expr, filename, value);
      }
      return value;
    }
  } else if (!right_node) {
    if (cubec_location_is(opt->loc, "++")) {
      return cubec_eval_self_opt(ctx, expr, filename, opt, false);
    } else if (cubec_location_is(opt->loc, "--")) {
      return cubec_eval_self_opt(ctx, expr, filename, opt, false);
    } else {
      return cubec_context_create_compile_error(ctx, expr, filename,
                                                "Invalid operator");
    }
  } else {
    cubec_value_t left = cubec_eval_expression(ctx, left_node, filename);
    if (left->type->kind == CUBEC_TYPE_KIND_ERROR) {
      return left;
    }
    cubec_value_t right = cubec_eval_expression(ctx, right_node, filename);
    if (right->type->kind == CUBEC_TYPE_KIND_ERROR) {
      return right;
    }
  }
  return cubec_context_create_compile_error(ctx, expr, filename,
                                            "Invalid expression");
}