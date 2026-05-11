#include "c/type.h"
#include "c/value.h"
#include "core/array.h"
#include "core/stream.h"
#include "engine/array.h"
#include "engine/function.h"
#include "engine/ptr.h"
#include "engine/slice.h"
#include "engine/struct.h"
#include "engine/type.h"
#include "engine/value.h"
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
  case TYPE_KIND_STRUCT:
  case TYPE_KIND_SLICE:
  case TYPE_KIND_UNION:
  case TYPE_KIND_ARRAY:
  case TYPE_KIND_FUNCTION: {
    stream_write(stream, type_get_id(type));
    break;
  }
  default:
    break;
  }
}

void write_c_type_declarator(context_t ctx, type_t type, stream_t stream) {
  stream_write(stream, "typedef struct _");
  stream_write(stream, type_get_id(type));
  stream_write(stream, " ");
  stream_write(stream, type_get_id(type));
  stream_write(stream, ";");
}
void write_c_type_declaration(context_t ctx, type_t type, stream_t stream) {
  if (type_get_kind(type) == TYPE_KIND_ARRAY) {
    size_t len = array_type_get_length(type);
    type_t base_type = array_type_get_type(type);
    stream_write(stream, "struct _");
    stream_write(stream, type_get_id(type));
    stream_write(stream, " {");
    stream_inc_indent(stream);
    stream_newline(stream);
    write_c_type(ctx, base_type, stream);
    size_t size = snprintf(NULL, 0, " items[%" PRIuPTR "];", len);
    char buf[size];
    sprintf(buf, " items[%" PRIuPTR "];", len);
    stream_write(stream, buf);
    stream_dec_indent(stream);
    stream_newline(stream);
    stream_write(stream, "};");
  } else if (type_get_kind(type) == TYPE_KIND_SLICE) {
    type_t base_type = slice_type_get_type(type);
    stream_write(stream, "struct _");
    stream_write(stream, type_get_id(type));
    stream_write(stream, " {");
    stream_inc_indent(stream);
    stream_newline(stream);
    if (!slice_type_is_mut(type)) {
      stream_write(stream, "const ");
    }
    write_c_type(ctx, base_type, stream);
    stream_write(stream, " *data;");
    stream_newline(stream);
    stream_write(stream, "size_t offset;");
    stream_newline(stream);
    stream_write(stream, "size_t length;");
    stream_dec_indent(stream);
    stream_newline(stream);
    stream_write(stream, "};");
  } else if (type_get_kind(type) == TYPE_KIND_STRUCT) {
    stream_write(stream, "struct _");
    stream_write(stream, type_get_id(type));
    stream_write(stream, " {");
    array_t fields = struct_type_get_fields(type);
    if (array_get_size(fields)) {
      stream_inc_indent(stream);
      for (size_t idx = 0; idx < array_get_size(fields); idx++) {
        struct_field_t field = array_get(fields, idx);
        stream_newline(stream);
        if (!field->mut) {
          stream_write(stream, "const ");
        }
        write_c_type(ctx, field->type, stream);
        stream_write(stream, " ");
        stream_write(stream, field->name);
        stream_write(stream, ";");
      }
      stream_dec_indent(stream);
      stream_newline(stream);
    }
    stream_write(stream, "}");
    if (struct_type_is_packed(type)) {
      stream_write(stream, " __attribute__((packed))");
    }
    if (struct_type_is_aligned(type)) {
      size_t len = snprintf(NULL, 0, " __attribute((aligned(%" PRIuPTR ")))",
                            type_get_align(type));
      char buf[len];
      sprintf(buf, " __attribute((aligned(%" PRIuPTR ")))",
              type_get_align(type));
      stream_write(stream, buf);
    }
    stream_write(stream, ";");
    array_t attributes = struct_type_get_attributes(type);
    for (size_t idx = 0; idx < array_get_size(attributes); idx++) {
      struct_attribute_t attr = array_get(attributes, idx);
      value_t val = attr->value;
      if (!attr->comptime) {
        stream_newline(stream);
        if (!value_is_mut(val)) {
          stream_write(stream, "const ");
        }
        type_t atype = value_get_type(val);
        write_c_type(ctx, atype, stream);
        stream_write(stream, " ");
        stream_write(stream, type_get_id(type));
        stream_write(stream, "A");
        stream_write(stream, attr->name);
        stream_write(stream, " = ");
        write_c_value(ctx, attr->value, stream);
        stream_write(stream, ";");
      }
    }
  } else if (type_get_kind(type) == TYPE_KIND_FUNCTION) {
    stream_write(stream, "typedef ");
    ctype_t ret = function_type_get_type(type);
    if (!ret->mut) {
      stream_write(stream, "const ");
    }
    write_c_type(ctx, ret->type, stream);
    stream_write(stream, "(*");
    stream_write(stream, type_get_id(type));
    stream_write(stream, ")(");
    array_t arguments = function_type_get_arguments(type);
    for (size_t idx = 0; idx < array_get_size(arguments); idx++) {
      if (idx != 0) {
        stream_write(stream, ", ");
      }
      ctype_t arg = array_get(arguments, idx);
      if (!arg->mut) {
        stream_write(stream, "const ");
      }
      if (idx == array_get_size(arguments) - 1) {
        if (function_type_is_variadic(type)) {
          if (arg->type) {
            type_t slice_type = create_slice_type(ctx, arg->type, arg->mut);
            write_c_type(ctx, slice_type, stream);
          } else {
            stream_write(stream, "...");
          }
        } else {
          write_c_type(ctx, arg->type, stream);
        }
      }
    }
    stream_write(stream, ");");
  }
}