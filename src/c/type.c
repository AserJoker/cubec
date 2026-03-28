#include "c/type.h"
#include "ast/node.h"
#include "core/array.h"
#include "core/string.h"
#include "engine/array.h"
#include "engine/context.h"
#include "engine/enum.h"
#include "engine/function.h"
#include "engine/ptr.h"
#include "engine/result.h"
#include "engine/struct.h"
#include "engine/type.h"
#include "engine/union.h"
#include "engine/value.h"
#include "eval/type.h"
#include <inttypes.h>
#include <stdio.h>

static void cubec_type_to_c(cubec_context_t ctx, cubec_type_t type,
                            cubec_string_t *output) {
  switch (type->kind) {
  case CUBEC_TYPE_KIND_VOID:
    cubec_string_concat(*output, ctx->allocator, "void");
    break;
  case CUBEC_TYPE_KIND_INT8:
    cubec_string_concat(*output, ctx->allocator, "int8_t");
    break;
  case CUBEC_TYPE_KIND_INT16:
    cubec_string_concat(*output, ctx->allocator, "int16_t");
    break;
  case CUBEC_TYPE_KIND_INT32:
    cubec_string_concat(*output, ctx->allocator, "int32_t");
    break;
  case CUBEC_TYPE_KIND_INT64:
    cubec_string_concat(*output, ctx->allocator, "int64_t");
    break;
  case CUBEC_TYPE_KIND_UINT8:
    cubec_string_concat(*output, ctx->allocator, "uint8_t");
    break;
  case CUBEC_TYPE_KIND_UINT16:
    cubec_string_concat(*output, ctx->allocator, "uint16_t");
    break;
  case CUBEC_TYPE_KIND_UINT32:
    cubec_string_concat(*output, ctx->allocator, "uint32_t");
    break;
  case CUBEC_TYPE_KIND_UINT64:
    cubec_string_concat(*output, ctx->allocator, "uint64_t");
    break;
  case CUBEC_TYPE_KIND_FLOAT32:
    cubec_string_concat(*output, ctx->allocator, "float");
    break;
  case CUBEC_TYPE_KIND_FLOAT64:
    cubec_string_concat(*output, ctx->allocator, "double");
    break;
  case CUBEC_TYPE_KIND_BOOLEAN:
    cubec_string_concat(*output, ctx->allocator, "bool");
    break;
  case CUBEC_TYPE_KIND_STR:
    cubec_string_concat(*output, ctx->allocator, "const char *");
    break;
  case CUBEC_TYPE_KIND_OPAQUE:
    cubec_string_concat(*output, ctx->allocator, "void *");
    break;
  case CUBEC_TYPE_KIND_PTR:
  case CUBEC_TYPE_KIND_PTR_ARRAY: {
    cubec_ptr_meta_t meta = type->meta;
    cubec_type_to_c(ctx, meta->type, output);
    cubec_string_concat(*output, ctx->allocator, " *");
    if (!meta->is_mutable) {
      cubec_string_concat(*output, ctx->allocator, " const");
    }
    if (!meta->is_volatile) {
      cubec_string_concat(*output, ctx->allocator, " volatile");
    }
    break;
  }
  case CUBEC_TYPE_KIND_ARRAY: {
    cubec_array_meta_t meta = type->meta;
    cubec_type_to_c(ctx, meta->type, output);
    char num[32];
    sprintf(num, "[%" PRIuPTR "]", meta->len);
    cubec_string_concat(*output, ctx->allocator, num);
    break;
  }
  case CUBEC_TYPE_KIND_ENUM: {
    cubec_enum_meta_t meta = type->meta;
    cubec_type_to_c(ctx, meta->type, output);
    break;
  }
  case CUBEC_TYPE_KIND_STRUCT: {
    cubec_struct_meta_t meta = type->meta;
    cubec_string_concat(*output, ctx->allocator, "struct ");
    // TODO:
    break;
  }
  case CUBEC_TYPE_KIND_UNION: {
    cubec_union_meta_t meta = type->meta;
    cubec_string_concat(*output, ctx->allocator, "union ");
    // TODO:
    break;
  }
  case CUBEC_TYPE_KIND_RESULT: {
    cubec_result_meta_t meta = type->meta;
    cubec_string_concat(*output, ctx->allocator, "struct ");
    // TODO:
    break;
  }
  case CUBEC_TYPE_KIND_FUNCTION: {
    cubec_function_meta_t meta = type->meta;
    cubec_type_to_c(ctx, meta->type, output);
    cubec_string_concat(*output, ctx->allocator, " (*)");
    cubec_string_concat(*output, ctx->allocator, "(");
    cubec_array_t args = meta->args;
    if (!cubec_array_get_size(args)) {
      cubec_string_concat(*output, ctx->allocator, "void");
    } else {
      for (size_t idx = 0; idx < cubec_array_get_size(args); idx++) {
        if (idx != 0) {
          cubec_string_concat(*output, ctx->allocator, ", ");
        }
        cubec_type_t arg_type = cubec_array_get(args, idx);
        cubec_type_to_c(ctx, arg_type, output);
      }
      if (meta->is_variadic) {
        cubec_string_concat(*output, ctx->allocator, ", ...");
      }
    }
    cubec_string_concat(*output, ctx->allocator, ")");
    break;
  }
  default:
    break;
  }
}

cubec_value_t cubec_c_write_type(cubec_context_t self, cubec_ast_node_t type,
                                 const char *filename, cubec_string_t *output) {
  cubec_value_t t = cubec_eval_type(self, type, filename);
  if (t->type->kind == CUBEC_TYPE_KIND_ERROR) {
    return t;
  }
  cubec_type_t tt = *(cubec_type_t *)t->data;
  cubec_type_to_c(self, tt, output);
  return self->value_undefined;
}