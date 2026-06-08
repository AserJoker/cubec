#include "c/value.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/stream.h"
#include "core/string.h"
#include "engine/arr.h"
#include "engine/context.h"
#include "engine/float.h"
#include "engine/function.h"
#include "engine/integer.h"
#include "engine/struct.h"
#include "engine/unsigned.h"
#include "engine/value.h"
#include <inttypes.h>
#include <stdbool.h>

void c_value(c_writer_t writer, value_t value) {
  type_t type = value->type;
  stream_t stream = writer->stream;
  switch (type->kind) {
  case TYPE_KIND_NIL:
    stream_write(writer->stream, "((void*)0)");
    break;
  case TYPE_KIND_BOOL: {
    bool val = *(bool *)value->data;
    stream_write(writer->stream, val ? "true" : "false");
  } break;
  case TYPE_KIND_STR: {
    const char *s = *(const char **)value->data;
    char *es = encode_cstring(writer->ctx->allocator, s);
    stream_write(writer->stream, "\"%s\"", es);
    allocator_free(writer->ctx->allocator, es);
  } break;
  case TYPE_KIND_I8:
  case TYPE_KIND_I16:
  case TYPE_KIND_I32:
  case TYPE_KIND_I64: {
    int64_t val = integer_get_value(value);
    stream_write(writer->stream, "%" PRIdPTR, val);
  } break;
  case TYPE_KIND_U8:
  case TYPE_KIND_U16:
  case TYPE_KIND_U32:
  case TYPE_KIND_U64: {
    uint64_t val = unsigned_get_value(value);
    stream_write(writer->stream, "%" PRIuPTR, val);
  } break;
  case TYPE_KIND_F16:
  case TYPE_KIND_F32:
  case TYPE_KIND_F64: {
    float64_t val = float_get_value(value);
    stream_write(writer->stream, "%g", val);
  } break;
  case TYPE_KIND_STRUCT: {
    array_t fields = struct_type_get_fields(type);
    uint8_t *data = value->data;
    stream_write(stream, "((%s){", type->id);
    if (array_get_size(fields)) {
      stream_inc_indent(stream);
      for (size_t idx = 0; idx < array_get_size(fields); idx++) {
        stream_newline(stream);
        struct_field_t field = array_get(fields, idx);
        value_t val = context_create_comptime_value(
            writer->ctx, field->type, data + field->offset, field->mut, NULL);
        stream_write(stream, ".%s = ", field->name);
        c_value(writer, val);
        stream_write(stream, ",");
      }
      stream_dec_indent(stream);
      stream_newline(stream);
    }
    stream_write(stream, "})");
  } break;
  case TYPE_KIND_ENUM: {

  } break;
  case TYPE_KIND_UNION: {

  } break;
  case TYPE_KIND_ARRAY: {
    size_t len = arr_type_get_length(type);
    type_t base_type = arr_type_get_type(type);
    stream_write(stream, "((%s){", type->id);
    stream_inc_indent(stream);
    stream_newline(stream);
    stream_write(stream, ".items = {");
    if (len) {
      stream_inc_indent(stream);
      for (size_t idx = 0; idx < len; idx++) {
        stream_newline(stream);
        value_t index = create_comptime_u64(writer->ctx, idx, false, NULL);
        value_t val = value_get(value, writer->ctx, index);
        c_value(writer, val);
        stream_write(stream, ",");
      }
      stream_dec_indent(stream);
      stream_newline(stream);
    }
    stream_write(stream, "},");
    stream_dec_indent(stream);
    stream_newline(stream);
    stream_write(stream, "})");
  } break;
  case TYPE_KIND_FUNCTION: {
    function_declar_t declar = *(function_declar_t *)value->data;
    stream_write(writer->stream, declar->id);
  } break;
  default:
    break;
  }
}