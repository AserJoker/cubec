#include "c/type.h"
#include "core/stream.h"
#include "engine/array.h"
#include "engine/ptr.h"
#include "engine/type.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

void write_c_type(context_t ctx, type_t type, stream_t stream) {
  const char *id = type_get_id(type);
  switch (type_get_kind(type)) {
  case TYPE_KIND_VOID:
    stream_write(stream, "void");
    break;
  case TYPE_KIND_NULL:
    stream_write(stream, "void*");
    break;
  case TYPE_KIND_INTEGER: {
    if (strcmp(id, "i8") == 0) {
      stream_write(stream, "int8_t");
    } else if (strcmp(id, "i16") == 0) {
      stream_write(stream, "int16_t");
    } else if (strcmp(id, "i32") == 0) {
      stream_write(stream, "int32_t");
    } else if (strcmp(id, "i64") == 0) {
      stream_write(stream, "int64_t");
    }
    break;
  }
  case TYPE_KIND_UNSIGNED: {
    if (strcmp(id, "u8") == 0) {
      stream_write(stream, "uint8_t");
    } else if (strcmp(id, "u16") == 0) {
      stream_write(stream, "uint16_t");
    } else if (strcmp(id, "u32") == 0) {
      stream_write(stream, "uint32_t");
    } else if (strcmp(id, "u64") == 0) {
      stream_write(stream, "uint64_t");
    }
    break;
  }
  case TYPE_KIND_FLOAT: {
    if (strcmp(id, "f32") == 0) {
      stream_write(stream, "float");
    } else if (strcmp(id, "f64") == 0) {
      stream_write(stream, "double");
    }
    break;
  }
  case TYPE_KIND_BOOL:
    stream_write(stream, "bool");
    break;
  case TYPE_KIND_STR:
    stream_write(stream, "const char*");
    break;
  case TYPE_KIND_PTR: {
    type_t base_type = ptr_type_get_type(type);
    write_c_type(ctx, base_type, stream);
    stream_write(stream, "*");
    break;
  }
  case TYPE_KIND_PARRAY: {
    type_t base_type = ptr_type_get_type(type);
    write_c_type(ctx, base_type, stream);
    stream_write(stream, "*");
    break;
  }
  case TYPE_KIND_OPAQUE: {
    stream_write(stream, "void*");
    break;
  }
  case TYPE_KIND_STRUCT: {
    stream_write(stream, "struct ");
    stream_write(stream, type_get_id(type));
    break;
  }
  case TYPE_KIND_SLICE: {
    break;
  }
  case TYPE_KIND_UNION: {
    stream_write(stream, "union ");
    stream_write(stream, type_get_id(type));
    break;
  }
  case TYPE_KIND_ARRAY: {
    type_t base_type = array_type_get_type(type);
    size_t len = array_type_get_length(type);
    write_c_type(ctx, base_type, stream);
    size_t size = snprintf(NULL, 0, "[%" PRIuPTR "]", len);
    char buf[size];
    sprintf(buf, "[%" PRIuPTR "]", len);
    stream_write(stream, buf);
    break;
  }
  case TYPE_KIND_FUNCTION: {
    break;
  }
  default:
    break;
  }
}