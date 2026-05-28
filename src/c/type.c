#include "c/type.h"
#include "core/array.h"
#include "core/hash_map.h"
#include "core/stream.h"
#include "engine/arr.h"
#include "engine/context.h"
#include "engine/function.h"
#include "engine/ptr.h"
#include "engine/slice.h"
#include "engine/type.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

void c_type_declarator(context_t ctx, type_t type, stream_t stream) {
  if (type->kind == TYPE_KIND_STRUCT || type->kind == TYPE_KIND_ARRAY ||
      type->kind == TYPE_KIND_SLICE) {
    stream_write(stream, "typedef struct _");
    stream_write(stream, type->id);
    stream_write(stream, " ");
    stream_write(stream, type->id);
    stream_write(stream, ";");
    stream_newline(stream);
  }
  if (type->kind == TYPE_KIND_FUNCTION) {
    stream_write(stream, "typedef struct _");
    stream_write(stream, type->id);
    stream_write(stream, "* ");
    stream_write(stream, type->id);
    stream_write(stream, ";");
    stream_newline(stream);
  }
}
void c_type_declaration(context_t ctx, type_t type, stream_t stream) {
  if (type->kind == TYPE_KIND_PTR) {
    type_t base_type = ptr_type_get_type(type);
    bool mut = ptr_type_is_mut(type);
    bool vol = ptr_type_is_vol(type);
    stream_write(stream, "typedef ");
    if (!mut) {
      stream_write(stream, "const ");
    }
    if (vol) {
      stream_write(stream, "volatile ");
    }
    c_type(ctx, base_type, stream);
    stream_write(stream, " * ");
    stream_write(stream, type->id);
    stream_write(stream, ";");
    stream_newline(stream);
  }
  if (type->kind == TYPE_KIND_ARRAY) {
    type_t base_type = arr_type_get_type(type);
    size_t length = arr_type_get_length(type);
    stream_write(stream, "struct _");
    stream_write(stream, type->id);
    stream_write(stream, " {");
    stream_inc_indent(stream);
    stream_newline(stream);
    c_type(ctx, base_type, stream);
    stream_write(stream, " items[");
    size_t len = snprintf(NULL, 0, "%" PRIuPTR, length);
    char buf[len + 1];
    sprintf(buf, "%" PRIuPTR, length);
    stream_write(stream, buf);
    stream_write(stream, "];");
    stream_dec_indent(stream);
    stream_newline(stream);
    stream_write(stream, "};");
    stream_newline(stream);
  }
  if (type->kind == TYPE_KIND_SLICE) {
    type_t base_type = slice_type_get_type(type);
    stream_write(stream, "struct _");
    stream_write(stream, type->id);
    stream_write(stream, " {");
    stream_inc_indent(stream);
    stream_newline(stream);
    c_type(ctx, base_type, stream);
    stream_write(stream, "* data;");
    stream_newline(stream);
    stream_write(stream, "uint64_t start;");
    stream_newline(stream);
    stream_write(stream, "uint64_t end;");
    stream_dec_indent(stream);
    stream_newline(stream);
    stream_write(stream, "};");
    stream_newline(stream);
  }
  if (type->kind == TYPE_KIND_FUNCTION) {
    function_meta_t meta = type->meta;
    stream_write(stream, "typedef struct _");
    stream_write(stream, type->id);
    stream_write(stream, "_CLOSURE* ");
    stream_write(stream, type->id);
    stream_write(stream, "_CLOSURE;");
    stream_newline(stream);
    stream_write(stream, "struct _");
    stream_write(stream, type->id);
    stream_write(stream, "_CLOSURE {");
    if (hash_map_get_size(meta->closure)) {
      stream_inc_indent(stream);
      stream_newline(stream);
      list_node_t it = hash_map_get_first(meta->closure);
      while (it != hash_map_get_end(meta->closure)) {
        if (it != hash_map_get_first(meta->closure)) {
          stream_newline(stream);
        }
        const char *key = hash_map_node_get_key(it);
        type_t type = hash_map_node_get_value(it);
        c_type(ctx, type, stream);
        stream_write(stream, " ");
        stream_write(stream, key);
        stream_write(stream, ";");
        it = hash_map_node_get_next(it);
      }
      stream_dec_indent(stream);
      stream_newline(stream);
    }
    stream_write(stream, "};");
    stream_newline(stream);

    stream_write(stream, "typedef ");
    if (!meta->type->mut) {
      stream_write(stream, "const ");
    }
    c_type(ctx, meta->type->type, stream);
    stream_write(stream, " (*");
    stream_write(stream, type->id);
    stream_write(stream, "_CALLEE");
    stream_write(stream, ")(");
    stream_write(stream, type->id);
    stream_write(stream, "_CLOSURE ");
    for (size_t idx = 0; idx < array_get_size(meta->args); idx++) {
      stream_write(stream, ", ");
      ctype_t arg = array_get(meta->args, idx);
      if (!arg->mut) {
        stream_write(stream, "const ");
      }
      c_type(ctx, arg->type, stream);
    }
    stream_write(stream, ");");
    stream_newline(stream);
    stream_write(stream, "struct _");
    stream_write(stream, type->id);
    stream_write(stream, " {");
    stream_inc_indent(stream);
    stream_newline(stream);
    stream_write(stream, type->id);
    stream_write(stream, "_CALLEE callee;");
    stream_newline(stream);
    stream_write(stream, type->id);
    stream_write(stream, "_CLOSURE closure;");
    stream_dec_indent(stream);
    stream_newline(stream);
    stream_write(stream, "};");
    stream_newline(stream);
  }
}

void c_type(context_t ctx, type_t type, stream_t stream) {
  switch (type->kind) {
  case TYPE_KIND_VOID:
    stream_write(stream, "void");
    break;
  case TYPE_KIND_BOOL:
    stream_write(stream, "bool");
    break;
  case TYPE_KIND_STR:
    stream_write(stream, "const char*");
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
  default:
    stream_write(stream, type->id);
    break;
  }
}