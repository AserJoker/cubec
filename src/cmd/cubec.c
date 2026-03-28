#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/path.h"
#include "core/string.h"
#include "engine/array.h"
#include "engine/context.h"
#include "engine/enum.h"
#include "engine/function.h"
#include "engine/module.h"
#include "engine/struct.h"
#include "engine/type.h"
#include "engine/union.h"
#include "engine/value.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

char *absolute(cubec_allocator_t allocator, const char *filename) {
  cubec_path_t path = cubec_create_path(allocator, filename);
  cubec_path_t abs = cubec_path_absolute(path, allocator);
  cubec_allocator_free(allocator, path);
  char *fullname = cubec_path_to_string(abs, allocator);
  cubec_allocator_free(allocator, abs);
  return fullname;
}

char *read(cubec_allocator_t allocator, const char *fullname) {
  FILE *fp = fopen(fullname, "rb");
  fseek(fp, 0, SEEK_END);
  size_t len = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  char *source = cubec_allocator_alloc(allocator, len + 1, NULL);
  fread(source, len, 1, fp);
  source[len] = 0;
  fclose(fp);
  return source;
}

void print_value(cubec_context_t ctx, cubec_value_t value) {
  switch (value->type->kind) {
  case CUBEC_TYPE_KIND_BOOLEAN:
    printf((*(bool *)value->data) ? "true" : "false");
    break;
  case CUBEC_TYPE_KIND_INT8:
    printf("%d", *(int8_t *)value->data);
    break;
  case CUBEC_TYPE_KIND_INT16:
    printf("%d", *(int16_t *)value->data);
    break;
  case CUBEC_TYPE_KIND_INT32:
    printf("%d", *(int32_t *)value->data);
    break;
  case CUBEC_TYPE_KIND_INT64:
    printf("%" PRIuPTR, *(int64_t *)value->data);
    break;
  case CUBEC_TYPE_KIND_UINT8:
    printf("%u", *(uint8_t *)value->data);
    break;
  case CUBEC_TYPE_KIND_UINT16:
    printf("%u", *(uint16_t *)value->data);
    break;
  case CUBEC_TYPE_KIND_UINT32:
    printf("%u", *(uint32_t *)value->data);
    break;
  case CUBEC_TYPE_KIND_UINT64:
    printf("%" PRIuPTR, *(uint64_t *)value->data);
    break;
  case CUBEC_TYPE_KIND_FLOAT32:
    printf("%g", *(float *)value->data);
    break;
  case CUBEC_TYPE_KIND_FLOAT64:
    printf("%g", *(double *)value->data);
    break;
  case CUBEC_TYPE_KIND_STR:
    printf("\"%s\"", *(const char **)value->data);
    break;
  case CUBEC_TYPE_KIND_OPAQUE:
    printf("opaque{0x%" PRIxPTR "}", (intptr_t)*(void **)value->data);
    break;
  case CUBEC_TYPE_KIND_PTR:
  case CUBEC_TYPE_KIND_PTR_ARRAY: {
    char *type_name = cubec_context_type_to_string(ctx, value->type);
    printf("%s{0x%" PRIxPTR "}", type_name, (intptr_t)*(void **)value->data);
    cubec_allocator_free(ctx->allocator, type_name);
    break;
  }
  case CUBEC_TYPE_KIND_ARRAY: {
    cubec_array_meta_t meta = value->type->meta;
    char *type_name = cubec_context_type_to_string(ctx, value->type);
    printf("%s{", type_name);
    for (size_t idx = 0; idx < meta->len; idx++) {
      if (idx != 0) {
        printf(", ");
      }
      cubec_value_t item = cubec_context_get_index(ctx, value, idx);
      print_value(ctx, item);
    }
    printf("}");
    cubec_allocator_free(ctx->allocator, type_name);
    break;
  }
  case CUBEC_TYPE_KIND_STRUCT: {
    cubec_struct_meta_t meta = value->type->meta;
    char *type_name = cubec_context_type_to_string(ctx, value->type);
    printf("%s{", type_name);
    for (size_t idx = 0; idx < cubec_array_get_size(meta->fields); idx++) {
      if (idx != 0) {
        printf(", ");
      }
      cubec_struct_field_t field = cubec_array_get(meta->fields, idx);
      printf("%s:", field->name);
      cubec_value_t item = cubec_context_get_field(ctx, value, field->name);
      print_value(ctx, item);
    }
    printf("}");
    cubec_allocator_free(ctx->allocator, type_name);
    break;
  }
  case CUBEC_TYPE_KIND_UNION: {
    cubec_union_meta_t meta = value->type->meta;
    char *type_name = cubec_context_type_to_string(ctx, value->type);
    printf("%s{", type_name);
    for (size_t idx = 0; idx < cubec_array_get_size(meta->fields); idx++) {
      if (idx != 0) {
        printf(", ");
      }
      cubec_union_field_t field = cubec_array_get(meta->fields, idx);
      printf("%s: ", field->name);
      cubec_value_t item = cubec_context_get_field(ctx, value, field->name);
      print_value(ctx, item);
    }
    printf("}");
    cubec_allocator_free(ctx->allocator, type_name);
    break;
  }
  case CUBEC_TYPE_KIND_ENUM: {
    char *type_name = cubec_context_type_to_string(ctx, value->type);
    uint32_t idx = *(uint32_t *)value->data;
    cubec_enum_meta_t meta = value->type->meta;
    cubec_enum_option_t opt = cubec_array_get(meta->options, idx);
    printf("%s{", type_name);
    printf("%s(", opt->name);
    cubec_value_t val =
        cubec_context_create_value(ctx, meta->type, false, opt->value, NULL);
    print_value(ctx, val);
    printf(")");
    printf("}");
    cubec_allocator_free(ctx->allocator, type_name);
    break;
  }
  case CUBEC_TYPE_KIND_RESULT: {
    char *type_name = cubec_context_type_to_string(ctx, value->type);
    printf("%s{", type_name);
    printf("}");
    cubec_allocator_free(ctx->allocator, type_name);
    break;
  }
  case CUBEC_TYPE_KIND_FUNCTION: {
    char *type_name = cubec_context_type_to_string(ctx, value->type);
    printf("%s{", type_name);
    printf("0x%" PRIxPTR, (intptr_t)*(void **)(value->data));
    printf("}");
    cubec_allocator_free(ctx->allocator, type_name);
    break;
  }
  case CUBEC_TYPE_KIND_TYPE: {
    cubec_type_t t = *(cubec_type_t *)value->data;
    char *type_name = cubec_context_type_to_string(ctx, t);
    printf("type{ kind: %" PRIuPTR ", name: \"%s\", size: %" PRIuPTR " }",
           (uint64_t)t->kind, type_name, t->size);
    cubec_allocator_free(ctx->allocator, type_name);
    break;
  }
  default:
    break;
  }
}

cubec_value_t print(cubec_context_t ctx, size_t argc, cubec_value_t argv[]) {
  for (size_t idx = 0; idx < argc; idx++) {
    cubec_value_t arg = argv[idx];
    if (idx != 0) {
      printf(", ");
    }
    print_value(ctx, arg);
  }
  printf("\n");
  return ctx->value_undefined;
}
static cubec_ast_node_t cubec_visit_unwrap_group(cubec_allocator_t allocator,
                                                 cubec_ast_node_t node,
                                                 cubec_context_t ctx) {
  if (node->type == CUBEC_NODE_TYPE_EXPRESSION_GROUP) {
    cubec_ast_node_t parent = node->parent;
    cubec_ast_node_t body = cubec_ast_move_child(allocator, node, "body");
    if (parent->type == CUBEC_NODE_TYPE_LIST) {
      size_t idx = cubec_ast_get_item_index(parent, node);
      cubec_ast_set_item(allocator, parent, idx, body);
    } else {
      const char *name = cubec_ast_get_child_name(parent, node);
      cubec_ast_set_child(allocator, parent, name, body);
    }
    return parent;
  }
  return node;
}
static cubec_visit_ast_fn_t visits[] = {
    (cubec_visit_ast_fn_t)cubec_visit_unwrap_group,
};
int main(int argc, char *argv[]) {
  cubec_allocator_t allocator = cubec_create_allocator(NULL);
  cubec_context_t ctx = cubec_create_context(allocator);
  ctx->num_visits = sizeof(visits) / sizeof(cubec_visit_ast_fn_t);
  ctx->visits = visits;
  cubec_context_create_int32(ctx, 0, true, "a");
  cubec_type_t print_fn_t =
      cubec_context_create_function_type(ctx, ctx->type_void, 0, NULL, true);
  cubec_context_create_native(ctx, print_fn_t, print, false, "print");
  char *filename = absolute(allocator, "./main.cubec");
  cubec_value_t err = cubec_context_load_module(ctx, filename);
  if (err->type == ctx->type_error) {
    const char *message = *(const char **)err->data;
    fprintf(stderr, "%s\n", message);
  } else {
    cubec_module_t mod = cubec_context_get_module(ctx, filename);
    const char *dst = cubec_string_get(mod->data);
    printf("%s\n", dst);
  }
  cubec_allocator_free(allocator, filename);
  cubec_allocator_free(allocator, ctx);
  cubec_delete_allocator(allocator);
  return 0;
}