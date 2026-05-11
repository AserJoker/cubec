#include "c/value.h"
#include "c/type.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/stream.h"
#include "core/string.h"
#include "engine/array.h"
#include "engine/context.h"
#include "engine/float.h"
#include "engine/integer.h"
#include "engine/struct.h"
#include "engine/type.h"
#include "engine/unsigned.h"
#include "engine/value.h"
#include <inttypes.h>
#include <stdio.h>

void write_c_value_sub(context_t ctx, value_t value, stream_t stream) {
  type_t type = value_get_type(value);
  switch (type_get_kind(type)) {
  case TYPE_KIND_NULL:
    stream_write(stream, "NULL");
    break;
  case TYPE_KIND_INTEGER: {
    int64_t val = integer_get_value(value);
    size_t len = snprintf(NULL, 0, "%" PRIdPTR, val);
    char buf[len];
    sprintf(buf, "%" PRIdPTR, val);
    stream_write(stream, buf);
    break;
  }
  case TYPE_KIND_UNSIGNED: {
    uint64_t val = unsigned_get_value(value);
    size_t len = snprintf(NULL, 0, "%" PRIuPTR, val);
    char buf[len];
    sprintf(buf, "%" PRIuPTR, val);
    stream_write(stream, buf);
    break;
  }
  case TYPE_KIND_FLOAT: {
    double val = float_get_value(value);
    size_t len = snprintf(NULL, 0, "%g", val);
    char buf[len];
    sprintf(buf, "%g", val);
    stream_write(stream, buf);
    break;
  }
  case TYPE_KIND_BOOL: {
    bool val = *(bool *)value_get_data(value);
    stream_write(stream, val ? "true" : "false");
    break;
  }
  case TYPE_KIND_STR: {
    const char *str = *(const char **)value_get_data(value);
    allocator_t allocator = context_get_allocator(ctx);
    char *s = encode_cstring(allocator, str);
    stream_write(stream, "\"");
    stream_write(stream, s);
    stream_write(stream, "\"");
    allocator_free(allocator, s);
    break;
  }
  case TYPE_KIND_ARRAY: {
    size_t length = array_type_get_length(type);
    stream_write(stream, "{");
    stream_inc_indent(stream);
    stream_newline(stream);
    stream_write(stream, ".items = {");
    if (length) {
      stream_inc_indent(stream);
      for (size_t idx = 0; idx < length; idx++) {
        stream_newline(stream);
        value_t item =
            value_get(value, ctx, create_comptime_u64(ctx, idx, false, NULL));
        write_c_value_sub(ctx, item, stream);
        stream_write(stream, ",");
      }
      stream_dec_indent(stream);
      stream_newline(stream);
    } else {
      stream_write(stream, "0");
    }
    stream_write(stream, "},");
    stream_dec_indent(stream);
    stream_newline(stream);
    stream_write(stream, "}");
    break;
  }
  case TYPE_KIND_STRUCT: {
    array_t fields = struct_type_get_fields(type);
    stream_write(stream, "{");
    size_t len = array_get_size(fields);
    if (len) {
      stream_inc_indent(stream);
      for (size_t idx = 0; idx < len; idx++) {
        stream_newline(stream);
        struct_field_t field = array_get(fields, idx);
        stream_write(stream, ".");
        stream_write(stream, field->name);
        stream_write(stream, " = ");
        value_t item = value_get_field(value, ctx, field->name);
        write_c_value_sub(ctx, item, stream);
        stream_write(stream, ",");
      }
      stream_dec_indent(stream);
      stream_newline(stream);
    } else {
      stream_write(stream, "0");
    }
    stream_write(stream, "}");
    break;
  }
  case TYPE_KIND_SLICE:
  case TYPE_KIND_UNION:
  case TYPE_KIND_FUNCTION:
    break;
  default:
    break;
  }
}
void write_c_value(context_t ctx, value_t value, stream_t stream) {
  type_t type = value_get_type(value);
  if (type_get_kind(type) == TYPE_KIND_ARRAY) {
    stream_write(stream, "(");
    if (!value_is_mut(value)) {
      stream_write(stream, "const ");
    }
    write_c_type(ctx, type, stream);
    stream_write(stream, ")");
  } else if (type_get_kind(type) == TYPE_KIND_STRUCT) {
    stream_write(stream, "(");
    if (!value_is_mut(value)) {
      stream_write(stream, "const ");
    }
    write_c_type(ctx, type, stream);
    stream_write(stream, ")");
  }
  return write_c_value_sub(ctx, value, stream);
}