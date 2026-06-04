#include "c/type.h"
#include "c/writer.h"
#include "core/array.h"
#include "core/hash_map.h"
#include "core/list.h"
#include "core/stream.h"
#include "engine/arr.h"
#include "engine/function.h"
#include "engine/ptr.h"
#include "engine/slice.h"
#include "engine/struct.h"
#include "engine/type.h"
#include <inttypes.h>
#include <stdbool.h>
void c_type_declarator(c_writer_t writer, type_t type) {
  switch (type->kind) {
  case TYPE_KIND_STRUCT:
    stream_write(writer->stream, "typedef struct _%s %s;", type->id, type->id);
    stream_newline(writer->stream);
    break;
  case TYPE_KIND_ENUM:
    stream_write(writer->stream, "typedef struct _%s %s;", type->id, type->id);
    stream_newline(writer->stream);
    break;
  case TYPE_KIND_UNION:
    break;
  case TYPE_KIND_SLICE:
    stream_write(writer->stream, "typedef struct _%s %s;", type->id, type->id);
    stream_newline(writer->stream);
    break;
  case TYPE_KIND_ARRAY:
    stream_write(writer->stream, "typedef struct _%s %s;", type->id, type->id);
    stream_newline(writer->stream);
    break;
  case TYPE_KIND_FUNCTION: {
    function_meta_t meta = type->meta;
    if (hash_map_get_size(meta->closure)) {
      stream_write(writer->stream, "typedef struct _%s_env %s_env;", type->id,
                   type->id);
      stream_newline(writer->stream);
      stream_write(writer->stream, "typedef struct _%s %s;", type->id,
                   type->id);
      stream_newline(writer->stream);
    }
  } break;
  default:
    break;
  }
}
void c_type_declaration(c_writer_t writer, type_t type) {
  stream_t stream = writer->stream;
  switch (type->kind) {
  case TYPE_KIND_STRUCT: {
    struct_meta_t meta = type->meta;
    stream_write(stream, "struct _%s{", type->id);
    if (array_get_size(meta->fields)) {
      stream_inc_indent(stream);
      stream_newline(stream);
      for (size_t idx = 0; idx < array_get_size(meta->fields); idx++) {
        if (idx != 0) {
          stream_newline(stream);
        }
        struct_field_t f = array_get(meta->fields, idx);
        if (!f->mut) {
          stream_write(stream, "const ");
        }
        c_type(writer, f->type);
        stream_write(stream, " %s;", f->name);
      }
      stream_dec_indent(stream);
      stream_newline(stream);
    }
    stream_write(stream, "};");
    stream_newline(stream);
  } break;
  case TYPE_KIND_ENUM:
    break;
  case TYPE_KIND_UNION:
    break;
  case TYPE_KIND_PTR:
  case TYPE_KIND_PARRAY: {
    type_t base_type = ptr_type_get_type(type);
    bool mut = ptr_type_is_mut(type);
    bool vol = ptr_type_is_vol(type);
    stream_write(stream, "#define %s ", type->id);
    c_type(writer, base_type);
    if (!mut) {
      stream_write(stream, " const");
    }
    if (vol) {
      stream_write(stream, " volatile");
    }
    stream_write(stream, " *");
    stream_newline(stream);
  } break;
  case TYPE_KIND_SLICE: {
    type_t base_type = slice_type_get_type(type);
    base_type = create_ptr_type(writer->ctx, base_type, true, false);
    stream_write(stream, "struct _%s{", type->id);
    stream_inc_indent(stream);
    stream_newline(stream);
    c_type(writer, base_type);
    stream_write(stream, " data;");
    stream_newline(stream);
    stream_write(stream, "uint64_t offset;");
    stream_newline(stream);
    stream_write(stream, "uint64_t length;");
    stream_dec_indent(stream);
    stream_newline(stream);
    stream_write(stream, "};");
    stream_newline(stream);
  } break;
  case TYPE_KIND_ARRAY: {
    type_t base_type = arr_type_get_type(type);
    stream_write(stream, "struct _%s{", type->id);
    stream_inc_indent(stream);
    stream_newline(stream);
    c_type(writer, base_type);
    stream_write(stream, " items[%" PRIuPTR "];", arr_type_get_length(type));
    stream_dec_indent(stream);
    stream_newline(stream);
    stream_write(stream, "};");
    stream_newline(stream);
  } break;
  case TYPE_KIND_FUNCTION: {
    function_meta_t meta = type->meta;
    stream_write(stream, "typedef ");
    if (!meta->type->mut) {
      stream_write(stream, "const ");
    }
    c_type(writer, meta->type->type);
    if (hash_map_get_size(meta->closure)) {
      stream_write(stream, "(* %s_callee)(", type->id);
      stream_write(stream, "%s_env *", type->id);
      if (array_get_size(meta->args)) {
        stream_write(stream, ", ");
      }
    } else {
      stream_write(stream, "(* %s)(", type->id);
    }
    for (size_t idx = 0; idx < array_get_size(meta->args); idx++) {
      if (idx != 0) {
        stream_write(stream, ", ");
      }
      ctype_t arg = array_get(meta->args, idx);
      if (!arg->type) {
        stream_write(stream, "...");
      } else {
        if (!arg->mut) {
          stream_write(stream, "const ");
        }
        if (meta->variadic && idx == array_get_size(meta->args) - 1) {
          type_t slice_type = create_slice_type(writer->ctx, arg->type);
          c_type(writer, slice_type);
        } else {
          c_type(writer, arg->type);
        }
      }
    }
    stream_write(stream, ");");
    stream_newline(stream);
    if (hash_map_get_size(meta->closure)) {
      stream_write(stream, "struct _%s_env {", type->id);
      stream_inc_indent(stream);
      list_node_t it = hash_map_get_first(meta->closure);
      while (it != hash_map_get_end(meta->closure)) {
        stream_newline(stream);
        const char *key = hash_map_node_get_key(it);
        type_t type = hash_map_node_get_value(it);
        c_type(writer, type);
        stream_write(stream, " %s;", key);
        it = hash_map_node_get_next(it);
      }
      stream_dec_indent(stream);
      stream_newline(stream);
      stream_write(stream, "};");
      stream_newline(stream);
      stream_write(stream, "struct _%s {", type->id);
      stream_inc_indent(stream);
      stream_newline(stream);
      stream_write(stream, "%s_callee callee;", type->id);
      if (hash_map_get_size(meta->closure)) {
        stream_newline(stream);
        stream_write(stream, "%s_env env;", type->id);
      }
      stream_dec_indent(stream);
      stream_newline(stream);
      stream_write(stream, "};");
      stream_newline(stream);
      stream_write(stream, "inline ");
      if (!meta->type->mut) {
        stream_write(stream, "const ");
      }
      c_type(writer, meta->type->type);
      stream_write(stream, " %s_call(%s fn, ", type->id, type->id);
      for (size_t idx = 0; idx < array_get_size(meta->args); idx++) {
        if (idx != 0) {
          stream_write(stream, ", ");
        }
        ctype_t arg = array_get(meta->args, idx);
        if (!arg->mut) {
          stream_write(stream, "const ");
        }
        if (meta->variadic && idx == array_get_size(meta->args) - 1) {
          type_t slice_type = create_slice_type(writer->ctx, arg->type);
          c_type(writer, slice_type);
        } else {
          c_type(writer, arg->type);
        }
        stream_write(stream, " arg%" PRIuPTR, idx);
      }
      stream_write(stream, "){");
      stream_inc_indent(stream);
      stream_newline(stream);
      stream_write(stream, "return fn.callee(&fn.env");
      for (size_t idx = 0; idx < array_get_size(meta->args); idx++) {
        stream_write(stream, ", ");
        stream_write(stream, "arg%" PRIuPTR, idx);
      }
      stream_write(stream, ");");
      stream_dec_indent(stream);
      stream_newline(stream);
      stream_write(stream, "}");
      stream_newline(stream);
    }
  } break;
  default:
    break;
  }
}
void c_type(c_writer_t writer, type_t type) {
  c_writer_add_type(writer, type);
  stream_t stream = writer->stream;
  switch (type->kind) {
  case TYPE_KIND_NIL:
    stream_write(stream, "void *");
    break;
  case TYPE_KIND_VOID:
    stream_write(stream, "void");
    break;
  case TYPE_KIND_BOOL:
    stream_write(stream, "bool");
    break;
  case TYPE_KIND_STR:
    stream_write(stream, "const char *");
    break;
  case TYPE_KIND_I8:
    stream_write(stream, "int8_t");
    break;
  case TYPE_KIND_I16:
    stream_write(stream, "int16_t");
    break;
  case TYPE_KIND_I32:
    stream_write(stream, "int32_t");
    break;
  case TYPE_KIND_I64:
    stream_write(stream, "int64_t");
    break;
  case TYPE_KIND_U8:
    stream_write(stream, "uint8_t");
    break;
  case TYPE_KIND_U16:
    stream_write(stream, "uint16_t");
    break;
  case TYPE_KIND_U32:
    stream_write(stream, "uint32_t");
    break;
  case TYPE_KIND_U64:
    stream_write(stream, "uint64_t");
    break;
  case TYPE_KIND_F16:
    stream_write(stream, "float16_t");
    break;
  case TYPE_KIND_F32:
    stream_write(stream, "float32_t");
    break;
  case TYPE_KIND_F64:
    stream_write(stream, "float64_t");
    break;
  case TYPE_KIND_STRUCT:
  case TYPE_KIND_ENUM:
  case TYPE_KIND_UNION:
  case TYPE_KIND_PTR:
  case TYPE_KIND_PARRAY:
  case TYPE_KIND_SLICE:
  case TYPE_KIND_ARRAY:
  case TYPE_KIND_FUNCTION:
    stream_write(stream, "%s", type->id);
    break;
  default:
    break;
  }
}