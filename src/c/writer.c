#include "c/writer.h"
#include "ast/node.h"
#include "c/expression.h"
#include "c/program.h"
#include "c/type.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/compare.h"
#include "core/hash.h"
#include "core/hash_map.h"
#include "core/list.h"
#include "core/path.h"
#include "core/stream.h"
#include "core/string.h"
#include "engine/arr.h"
#include "engine/context.h"
#include "engine/function.h"
#include "engine/ptr.h"
#include "engine/slice.h"
#include "engine/struct.h"
#include "engine/type.h"
#include "engine/value.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
static void c_global_dispose(c_global_t self, allocator_t allocator) {
  allocator_free(allocator, self->name);
}

c_global_t create_c_global(allocator_t allocator, const char *name,
                           ast_node_t initialize, type_t type, bool mut) {
  c_global_t self = allocator_alloc(allocator, sizeof(struct _c_global_t),
                                    (dispose_fn_t)c_global_dispose);
  self->initialize = initialize;
  self->mut = mut;
  self->name = create_cstring(allocator, name);
  self->type = type;
  return self;
}
static void c_writer_dispose(c_writer_t self, allocator_t allocator) {
  allocator_free(allocator, self->types);
  allocator_free(allocator, self->globals);
  allocator_free(allocator, self->functions);
  allocator_free(allocator, self->modules);
}
c_writer_t create_c_writer(context_t ctx, stream_t stream) {
  c_writer_t self = allocator_alloc(ctx->allocator, sizeof(struct _c_writer_t),
                                    (dispose_fn_t)c_writer_dispose);
  self->ctx = ctx;
  self->types = create_array(ctx->allocator, NULL);
  self->globals = create_array(ctx->allocator, &(array_initialize_t){
                                                   .autofree = true,
                                               });
  self->functions = create_array(ctx->allocator, NULL);
  self->modules =
      create_hash_map(ctx->allocator, &(hash_map_initialize_t){
                                          .hash = (hash_fn_t)cstring_sdb,
                                          .compare = (compare_fn_t)strcmp,
                                          .autofree_key = false,
                                          .autofree_value = false,
                                      });
  self->stream = stream;
  return self;
}
void c_writer_add_type(c_writer_t self, type_t type) {
  if (type->comptime) {
    return;
  }
  switch (type->kind) {
  case TYPE_KIND_ENUM: {
    break;
  }
  case TYPE_KIND_PTR: {
    type_t base_type = ptr_type_get_type(type);
    c_writer_add_type(self, base_type);
    break;
  }
  case TYPE_KIND_SLICE: {
    type_t base_type = slice_type_get_type(type);
    base_type = create_ptr_type(self->ctx, base_type, false, false);
    c_writer_add_type(self, base_type);
    break;
  }
  case TYPE_KIND_ARRAY: {
    type_t base_type = arr_type_get_type(type);
    c_writer_add_type(self, base_type);
    break;
  }
  case TYPE_KIND_FUNCTION: {
    function_meta_t meta = type->meta;
    if (meta->closure) {
      list_node_t it = hash_map_get_first(meta->closure);
      while (it != hash_map_get_end(meta->closure)) {
        type_t type = hash_map_node_get_value(it);
        c_writer_add_type(self, type);
        it = hash_map_node_get_next(it);
      }
    }
    for (size_t idx = 0; idx < array_get_size(meta->args); idx++) {
      ctype_t type = array_get(meta->args, idx);
      if (type->type) {
        if (idx == array_get_size(meta->args) - 1 && meta->variadic) {
          type_t slice_type = create_slice_type(self->ctx, type->type);
          c_writer_add_type(self, slice_type);
        } else {
          c_writer_add_type(self, type->type);
        }
      }
    }
    c_writer_add_type(self, meta->type->type);
    break;
  }
  default:
    break;
  }
  for (size_t idx = 0; idx < array_get_size(self->types); idx++) {
    type_t t = array_get(self->types, idx);
    if (strcmp(t->id, type->id) == 0) {
      return;
    }
  }
  array_push(self->types, type);
  if (type->kind == TYPE_KIND_STRUCT) {
    array_t fields = struct_type_get_fields(type);
    for (size_t idx = 0; idx < array_get_size(fields); idx++) {
      struct_field_t f = array_get(fields, idx);
      c_writer_add_type(self, f->type);
    }
    hash_map_t attrs = struct_type_get_attributes(type);
    list_node_t it = hash_map_get_first(attrs);
    while (it != hash_map_get_end(attrs)) {
      const char *key = hash_map_node_get_key(it);
      struct_attribute_t attr = hash_map_node_get_value(it);
      if (!attr->comptime) {
        size_t len = snprintf(NULL, 0, "%s_%s", type->id, key);
        char name[len + 1];
        sprintf(name, "%s_%s", type->id, key);
        c_writer_add_global(self, name, attr->initialize, attr->type,
                            attr->mut);
      }
      it = hash_map_node_get_next(it);
    }
  }
}
void c_writer_add_function(c_writer_t self, value_t function) {
  c_writer_add_type(self, function->type);
  array_push(self->functions, function);
}
void c_writer_add_global(c_writer_t self, const char *name,
                         ast_node_t initialize, type_t type, bool mut) {
  c_writer_add_type(self, type);
  size_t len = snprintf(NULL, 0, "%s_%s", self->ctx->self->name, name);
  char id[len + 1];
  sprintf(id, "%s_%s", self->ctx->self->name, name);
  c_global_t global =
      create_c_global(self->ctx->allocator, id, initialize, type, mut);
  array_push(self->globals, global);
}
void c_writer_import(c_writer_t writer, const char *filename) {
  context_t ctx = writer->ctx;
  module_t current = writer->ctx->mod;
  path_t dir = create_path(ctx->allocator, ctx->mod->dirname);
  path_t fname = create_path(ctx->allocator, filename);
  path_t full = path_concat(dir, ctx->allocator, fname);
  char *fullname = path_to_string(full, ctx->allocator);
  allocator_free(ctx->allocator, full);
  allocator_free(ctx->allocator, fname);
  allocator_free(ctx->allocator, dir);
  if (!hash_map_get(writer->modules, fullname, NULL, NULL)) {
    module_t mod = hash_map_get(ctx->modules, fullname, NULL, NULL);
    hash_map_set(writer->modules, mod->filename, mod, NULL, NULL);
    ctx->mod = mod;
    c_program(writer, mod->doc->node);
    ctx->mod = current;
  }
  allocator_free(ctx->allocator, fullname);
}

void c_write_global(c_writer_t writer, c_global_t global) {
  if (!global->mut) {
    stream_write(writer->stream, "const ");
  }
  c_type(writer, global->type);
  stream_write(writer->stream, " %s = ", global->name);
  c_expression(writer, global->initialize);
  stream_write(writer->stream, ";");
  stream_newline(writer->stream);
}
void c_writer_write(c_writer_t writer) {
  stream_t stream = writer->stream;
  stream_write(stream, "#include <stdbool.h>");
  stream_newline(stream);
  stream_write(stream, "#include <stdint.h>");
  stream_newline(stream);
  stream_write(stream, "typedef _Float16 float16_t;");
  stream_newline(stream);
  stream_write(stream, "typedef float float32_t;");
  stream_newline(stream);
  stream_write(stream, "typedef double float64_t;");
  stream_newline(stream);
  for (size_t idx = 0; idx < array_get_size(writer->types); idx++) {
    type_t type = array_get(writer->types, idx);
    c_type_declarator(writer, type);
  }
  for (size_t idx = 0; idx < array_get_size(writer->types); idx++) {
    type_t type = array_get(writer->types, idx);
    c_type_declaration(writer, type);
  }
  for (size_t idx = 0; idx < array_get_size(writer->globals); idx++) {
    c_global_t global = array_get(writer->globals, idx);
    c_write_global(writer, global);
  }
  for (size_t idx = 0; idx < array_get_size(writer->functions); idx++) {
    value_t func = array_get(writer->functions, idx);
    function_declar_t declar = *(function_declar_t *)func->data;
    printf("function %s %s\n", declar->id, func->type->id);
  }
}