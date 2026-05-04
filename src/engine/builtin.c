#include "ast/node.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/string.h"
#include "engine/array.h"
#include "engine/buitin.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/float.h"
#include "engine/integer.h"
#include "engine/struct.h"
#include "engine/type.h"
#include "engine/unsigned.h"
#include "engine/value.h"
#include "resolve/expression.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static string_t value_to_str(context_t ctx, value_t self) {
  allocator_t allocator = context_get_allocator(ctx);
  string_t str = create_string(allocator, NULL);
  type_t type = value_get_type(self);
  if (!value_is_comptime(self)) {
    size_t len = snprintf(NULL, 0, "%s{runtime}", type_get_name(type));
    char buf[len];
    sprintf(buf, "%s{runtime}", type_get_name(type));
    string_concat(str, allocator, buf);
  } else {
    void *data = value_get_data(self);
    switch (type_get_kind(type)) {
    case TYPE_KIND_VOID:
      string_concat(str, allocator, "undefined");
      break;
    case TYPE_KIND_NULL:
      string_concat(str, allocator, "nil");
      break;
    case TYPE_KIND_TYPE:
      string_concat(str, allocator,
                    type_get_name(*(type_t *)value_get_data(self)));
      break;
    case TYPE_KIND_INTEGER: {
      size_t len = snprintf(NULL, 0, "%" PRIdPTR, integer_get_value(self));
      char buf[len];
      sprintf(buf, "%" PRIdPTR, integer_get_value(self));
      string_concat(str, allocator, buf);
      break;
    }
    case TYPE_KIND_UNSIGNED: {
      size_t len = snprintf(NULL, 0, "%" PRIuPTR, unsigned_get_value(self));
      char buf[len];
      sprintf(buf, "%" PRIuPTR, unsigned_get_value(self));
      string_concat(str, allocator, buf);
      break;
    }
    case TYPE_KIND_FLOAT: {
      size_t len = snprintf(NULL, 0, "%g", float_get_value(self));
      char buf[len];
      sprintf(buf, "%g", float_get_value(self));
      string_concat(str, allocator, buf);
      break;
    }
    case TYPE_KIND_BOOL: {
      string_concat(str, allocator, *(bool *)data ? "true" : "false");
      break;
    }
    case TYPE_KIND_STR: {
      string_concat(str, allocator, "\"");
      const char *src = *(const char **)data;
      char *encode_str = encode_cstring(allocator, src);
      string_concat(str, allocator, encode_str);
      allocator_free(allocator, encode_str);
      string_concat(str, allocator, "\"");
      break;
    }
    case TYPE_KIND_PTR:
    case TYPE_KIND_PARRAY:
    case TYPE_KIND_OPAQUE: {
      size_t len =
          snprintf(NULL, 0, "%s{%p}", type_get_name(type), *(void **)data);
      char s[len];
      sprintf(s, "%s{%p}", type_get_name(type), *(void **)data);
      string_concat(str, allocator, s);
      break;
    }
    case TYPE_KIND_ARRAY: {
      size_t size = array_type_get_length(type);
      string_concat(str, allocator, type_get_name(type));
      string_concat(str, allocator, "[");
      for (size_t idx = 0; idx < size; idx++) {
        if (idx != 0) {
          string_concat(str, allocator, ", ");
        }
        value_t item =
            value_get(self, ctx, create_comptime_u64(ctx, idx, false, NULL));
        string_t item_str = value_to_str(ctx, item);
        const char *cstr = string_get(item_str);
        string_concat(str, allocator, cstr);
        allocator_free(allocator, item_str);
      }
      string_concat(str, allocator, "]");
      break;
    }
    case TYPE_KIND_STRUCT: {
      array_t fields = struct_type_get_fields(type);
      string_concat(str, allocator, type_get_name(type));
      string_concat(str, allocator, "{");
      void *data = value_get_data(self);
      for (size_t idx = 0; idx < array_get_size(fields); idx++) {
        if (idx != 0) {
          string_concat(str, allocator, ", ");
        }
        struct_field_t field = array_get(fields, idx);
        string_concat(str, allocator, field->name);
        string_concat(str, allocator, " = ");
        value_t item = context_create_weak_value(
            ctx, field->type, (uint8_t *)data + field->offset, false, NULL);
        string_t field_str = value_to_str(ctx, item);
        const char *field_cstr = string_get(field_str);
        string_concat(str, allocator, field_cstr);
        allocator_free(allocator, field_str);
      }
      string_concat(str, allocator, "}");
      break;
    }
    case TYPE_KIND_UNION:
    default: {
      size_t len = snprintf(NULL, 0, "%s{%p}", type_get_name(type), data);
      char buf[len];
      sprintf(buf, "%s{%p}", type_get_name(type), data);
      string_concat(str, allocator, buf);
    } break;
    }
  }
  return str;
}

ast_node_t builtin_error(context_t ctx, size_t argc, ast_node_t *argv) {
  allocator_t allocator = context_get_allocator(ctx);
  if (argc < 1) {
    value_t err = create_error(ctx, "__error__ require fmt argument");
    return create_ast_value_node(allocator, err);
  }
  value_t fmt = resolve_expression(ctx, argv[0]);
  if (value_is_error(fmt) || value_is_interrupt(fmt)) {
    return create_ast_value_node(allocator, fmt);
  }
  type_t fmt_type = value_get_type(fmt);
  if (type_get_kind(fmt_type) != TYPE_KIND_STR) {
    value_t err = create_error(ctx, "cannot convert '%s' to 'str'",
                               type_get_name(fmt_type));
    return create_ast_value_node(allocator, err);
  }
  const char *format = *(const char **)value_get_data(fmt);
  value_t err = create_error(ctx, format);
  return create_ast_value_node(allocator, err);
}

ast_node_t builtin_typeof(context_t ctx, size_t argc, ast_node_t *argv) {
  allocator_t allocator = context_get_allocator(ctx);
  if (argc != 1) {
    value_t err = create_error(ctx, "__typeof__ require 1 argument");
    return create_ast_value_node(allocator, err);
  }
  value_t value = resolve_expression(ctx, argv[0]);
  if (value_is_error(value) || value_is_interrupt(value)) {
    return create_ast_value_node(allocator, value);
  }
  type_t type = value_get_type(value);
  value = create_type_value(ctx, type, false, NULL);
  return create_ast_value_node(allocator, value);
}

ast_node_t builtin_print(context_t ctx, size_t argc, ast_node_t *argv) {
  allocator_t allocator = context_get_allocator(ctx);
  if (argc < 1) {
    value_t err = create_error(ctx, "__print__ require 1 argument");
    return create_ast_value_node(allocator, err);
  }
  value_t fmt = resolve_expression(ctx, argv[0]);
  if (value_is_error(fmt) || value_is_interrupt(fmt)) {
    return create_ast_value_node(allocator, fmt);
  }
  type_t fmt_type = value_get_type(fmt);
  if (type_get_kind(fmt_type) != TYPE_KIND_STR) {
    value_t err = create_error(ctx, "cannot convert '%s' to 'str'",
                               type_get_name(fmt_type));
    return create_ast_value_node(allocator, err);
  }
  const char *format = *(const char **)value_get_data(fmt);
  string_t str = create_string(allocator, NULL);
  size_t offset = 1;
  const char *pfmt = format;
  while (*pfmt) {
    if (*pfmt == '%') {
      pfmt++;
      if (*pfmt == 'v') {
        if (offset >= argc) {
          string_concat(str, allocator, "%v");
        } else {
          value_t val = resolve_expression(ctx, argv[offset++]);
          if (value_is_error(val)) {
            allocator_free(allocator, str);
            return create_ast_value_node(allocator, val);
          }
          string_t s = value_to_str(ctx, val);
          string_concat(str, allocator, string_get(s));
          allocator_free(allocator, s);
        }
      }
    } else {
      char s[2] = {*pfmt, 0};
      string_concat(str, allocator, s);
    }
    pfmt++;
  }
  const char *cstr = string_get(str);
  printf("%s\n", cstr);
  allocator_free(allocator, str);
  return create_ast_value_node(allocator, context_get_undefined(ctx));
}