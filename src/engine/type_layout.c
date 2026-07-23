#include "engine/type_layout.h"
#include "engine/symbol.h"
#include "core/vec.h"
#include <stddef.h>

static size_t _align_up(size_t value, size_t alignment) {
  return (value + alignment - 1) & ~(alignment - 1);
}

static void _layout_compute_impl(semantic_type_t type, size_t ptr_size);

static size_t _primitive_size(enum type_kind kind) {
  switch (kind) {
  case TYPE_VOID:   return 0;
  case TYPE_BOOL:   return 1;
  case TYPE_I8:     return 1;
  case TYPE_U8:     return 1;
  case TYPE_I16:    return 2;
  case TYPE_U16:    return 2;
  case TYPE_F16:    return 2;
  case TYPE_I32:    return 4;
  case TYPE_U32:    return 4;
  case TYPE_F32:    return 4;
  case TYPE_I64:    return 8;
  case TYPE_U64:    return 8;
  case TYPE_F64:    return 8;
  case TYPE_CHAR:   return 1;
  case TYPE_STRING: return sizeof(void *); /* slice-like: ptr + len */
  case TYPE_STR:    return sizeof(void *); /* same layout as string */
  case TYPE_NIL:    return 0;
  case TYPE_MODULE: return 0;
  case TYPE_ERROR:  return 0;
  default:          return 0;
  }
}

static size_t _primitive_alignment(enum type_kind kind) {
  return _primitive_size(kind);
}

void type_layout_compute(semantic_type_t type, size_t ptr_size) {
  if (!type || !type->impl) return;

  type_impl_t impl = type->impl;

  /* Already computed */
  if (!type->is_incomplete && impl->size != 0) return;

  switch (impl->kind) {
  case TYPE_VOID: case TYPE_BOOL:
  case TYPE_I8: case TYPE_I16: case TYPE_I32: case TYPE_I64:
  case TYPE_U8: case TYPE_U16: case TYPE_U32: case TYPE_U64:
  case TYPE_F16: case TYPE_F32: case TYPE_F64:
  case TYPE_CHAR: case TYPE_NIL: case TYPE_ERROR:
    impl->size = _primitive_size(impl->kind);
    impl->alignment = _primitive_alignment(impl->kind);
    type->is_incomplete = false;
    break;

  case TYPE_STRING:
    /* string = { ptr, len } = 2 * ptr_size */
    impl->size = 2 * ptr_size;
    impl->alignment = ptr_size;
    type->is_incomplete = false;
    break;

  case TYPE_STR:
    /* str = compile-time string, same layout as string */
    impl->size = 2 * ptr_size;
    impl->alignment = ptr_size;
    type->is_incomplete = false;
    break;

  case TYPE_POINTER:
    impl->size = ptr_size;
    impl->alignment = ptr_size;
    type->is_incomplete = false;
    break;

  case TYPE_OPAQUE:
    impl->size = ptr_size;
    impl->alignment = ptr_size;
    type->is_incomplete = false;
    break;

  case TYPE_SLICE:
    /* slice = { ptr, len } = 2 * ptr_size */
    impl->size = 2 * ptr_size;
    impl->alignment = ptr_size;
    type->is_incomplete = false;
    break;

  case TYPE_ARRAY: {
    if (impl->array.length_param_name != NULL) {
      /* Symbolic length — cannot compute layout yet */
      impl->size = 0;
      impl->alignment = 0;
      type->is_incomplete = true;
      break;
    }
    _layout_compute_impl(impl->array.element, ptr_size);
    size_t elem_size = semantic_type_get_size(impl->array.element);
    size_t elem_align = semantic_type_get_alignment(impl->array.element);
    impl->size = elem_size * impl->array.length;
    impl->alignment = elem_align;
    type->is_incomplete = false;
    break;
  }

  case TYPE_STRUCT: {
    size_t offset = 0;
    size_t max_align = 1;
    vec_t fields = impl->struct_type.fields;
    size_t count = vec_get_size(fields);
    for (size_t i = 0; i < count; i++) {
      struct symbol *field = (struct symbol *)vec_get(fields, i);
      if (!field->field.type) continue;
      _layout_compute_impl(field->field.type, ptr_size);
      size_t fsize = semantic_type_get_size(field->field.type);
      size_t falign = semantic_type_get_alignment(field->field.type);
      if (falign == 0) falign = 1;
      if (!impl->is_packed) {
        offset = _align_up(offset, falign);
      }
      field->field.offset = offset;
      offset += fsize;
      if (falign > max_align) max_align = falign;
    }
    if (impl->explicit_align > 0 && impl->explicit_align > max_align) {
      max_align = impl->explicit_align;
    }
    if (!impl->is_packed) {
      offset = _align_up(offset, max_align);
    }
    /* Empty struct: minimum size 1 (like GCC/Clang) */
    if (offset == 0) offset = 1;
    impl->size = offset;
    impl->alignment = max_align;
    type->is_incomplete = false;
    break;
  }

  case TYPE_TUPLE: {
    size_t offset = 0;
    size_t max_align = 1;
    vec_t fields = impl->tuple.fields;
    size_t count = fields ? vec_get_size(fields) : 0;
    for (size_t i = 0; i < count; i++) {
      struct symbol *field = (struct symbol *)vec_get(fields, i);
      if (!field || !field->field.type) continue;
      _layout_compute_impl(field->field.type, ptr_size);
      size_t fsize = semantic_type_get_size(field->field.type);
      size_t falign = semantic_type_get_alignment(field->field.type);
      if (falign == 0) falign = 1;
      offset = _align_up(offset, falign);
      field->field.offset = offset;
      offset += fsize;
      if (falign > max_align) max_align = falign;
    }
    offset = _align_up(offset, max_align);
    if (offset == 0) offset = 1;
    impl->size = offset;
    impl->alignment = max_align;
    type->is_incomplete = false;
    break;
  }

  case TYPE_UNION: {
    /* Tagged union: layout is [tag: u64][data: max_field_size].
       __type__ is not a named field; tag occupies offset 0-7,
       field data starts at offset 8. */
    size_t max_size = 0;
    size_t max_align = 1;
    vec_t fields = impl->struct_type.fields;
    size_t count = vec_get_size(fields);
    for (size_t i = 0; i < count; i++) {
      struct symbol *field = (struct symbol *)vec_get(fields, i);
      if (!field->field.type) continue;
      _layout_compute_impl(field->field.type, ptr_size);
      size_t fsize = semantic_type_get_size(field->field.type);
      size_t falign = semantic_type_get_alignment(field->field.type);
      if (falign == 0) falign = 1;
      /* Fields start at offset 8 (after the u64 tag) */
      field->field.offset = 8;
      if (fsize > max_size) max_size = fsize;
      if (falign > max_align) max_align = falign;
    }
    if (impl->explicit_align > 0 && impl->explicit_align > max_align) {
      max_align = impl->explicit_align;
    }
    /* Tag requires at least 8-byte alignment */
    if (max_align < 8) max_align = 8;
    impl->size = 8 + _align_up(max_size, max_align); /* 8 for tag + aligned data */
    impl->alignment = max_align;
    type->is_incomplete = false;
    break;
  }

  case TYPE_CUNION: {
    /* C-style untagged union: size = max_field_size, no tag */
    size_t max_size = 0;
    size_t max_align = 1;
    vec_t fields = impl->struct_type.fields;
    size_t count = vec_get_size(fields);
    for (size_t i = 0; i < count; i++) {
      struct symbol *field = (struct symbol *)vec_get(fields, i);
      if (!field->field.type) continue;
      _layout_compute_impl(field->field.type, ptr_size);
      size_t fsize = semantic_type_get_size(field->field.type);
      size_t falign = semantic_type_get_alignment(field->field.type);
      if (falign == 0) falign = 1;
      field->field.offset = 0;
      if (fsize > max_size) max_size = fsize;
      if (falign > max_align) max_align = falign;
    }
    if (impl->explicit_align > 0 && impl->explicit_align > max_align) {
      max_align = impl->explicit_align;
    }
    max_size = _align_up(max_size, max_align);
    impl->size = max_size;
    impl->alignment = max_align;
    type->is_incomplete = false;
    break;
  }

  case TYPE_ENUM: {
    if (impl->enum_type.backing_type) {
      _layout_compute_impl(impl->enum_type.backing_type, ptr_size);
      impl->size = semantic_type_get_size(impl->enum_type.backing_type);
      impl->alignment = semantic_type_get_alignment(impl->enum_type.backing_type);
    } else {
      /* Default to i32 */
      impl->size = 4;
      impl->alignment = 4;
    }
    type->is_incomplete = false;
    break;
  }

  case TYPE_INTERFACE:
    /* Interface has no runtime representation */
    impl->size = 0;
    impl->alignment = 0;
    type->is_incomplete = false;
    break;

  case TYPE_FUNCTION:
    /* Function type itself has no size (it's not a function pointer) */
    impl->size = 0;
    impl->alignment = 0;
    type->is_incomplete = false;
    break;

  case TYPE_QUALIFIER:
    _layout_compute_impl(impl->qualifier.base, ptr_size);
    impl->size = semantic_type_get_size(impl->qualifier.base);
    impl->alignment = semantic_type_get_alignment(impl->qualifier.base);
    type->is_incomplete = false;
    break;

  case TYPE_TYPE:
    /* typeof type — size/alignment of the type descriptor itself */
    impl->size = 0;
    impl->alignment = 0;
    type->is_incomplete = false;
    break;

  case TYPE_GENERIC_INSTANCE: {
    /* Generic instances of struct/union have substituted fields */
    semantic_type_t tmpl = impl->generic_instance.generic_template;
    vec_t fields = impl->generic_instance.fields;
    if (!fields || vec_get_size(fields) == 0) break;

    if (tmpl && (tmpl->impl->kind == TYPE_STRUCT)) {
      size_t offset = 0;
      size_t max_align = 1;
      size_t count = vec_get_size(fields);
      for (size_t i = 0; i < count; i++) {
        struct symbol *field = (struct symbol *)vec_get(fields, i);
        if (!field->field.type) continue;
        _layout_compute_impl(field->field.type, ptr_size);
        size_t fsize = semantic_type_get_size(field->field.type);
        size_t falign = semantic_type_get_alignment(field->field.type);
        if (falign == 0) falign = 1;
        if (!impl->is_packed) {
          offset = _align_up(offset, falign);
        }
        field->field.offset = offset;
        offset += fsize;
        if (falign > max_align) max_align = falign;
      }
      if (!impl->is_packed) {
        offset = _align_up(offset, max_align);
      }
      if (offset == 0) offset = 1;
      impl->size = offset;
      impl->alignment = max_align;
      type->is_incomplete = false;
    } else if (tmpl && tmpl->impl->kind == TYPE_UNION) {
      /* Tagged union generic instance: same layout as TYPE_UNION */
      size_t max_size = 0;
      size_t max_align = 1;
      size_t count = vec_get_size(fields);
      for (size_t i = 0; i < count; i++) {
        struct symbol *field = (struct symbol *)vec_get(fields, i);
        if (!field->field.type) continue;
        _layout_compute_impl(field->field.type, ptr_size);
        size_t fsize = semantic_type_get_size(field->field.type);
        size_t falign = semantic_type_get_alignment(field->field.type);
        if (falign == 0) falign = 1;
        /* Fields start at offset 8 (after the u64 tag) */
        field->field.offset = 8;
        if (fsize > max_size) max_size = fsize;
        if (falign > max_align) max_align = falign;
      }
      if (max_align < 8) max_align = 8;
      impl->size = 8 + _align_up(max_size, max_align); /* 8 for tag + aligned data */
      impl->alignment = max_align;
      type->is_incomplete = false;
    } else if (tmpl && tmpl->impl->kind == TYPE_CUNION) {
      /* C-style untagged union generic instance */
      size_t max_size = 0;
      size_t max_align = 1;
      size_t count = vec_get_size(fields);
      for (size_t i = 0; i < count; i++) {
        struct symbol *field = (struct symbol *)vec_get(fields, i);
        if (!field->field.type) continue;
        _layout_compute_impl(field->field.type, ptr_size);
        size_t fsize = semantic_type_get_size(field->field.type);
        size_t falign = semantic_type_get_alignment(field->field.type);
        if (falign == 0) falign = 1;
        field->field.offset = 0;
        if (fsize > max_size) max_size = fsize;
        if (falign > max_align) max_align = falign;
      }
      max_size = _align_up(max_size, max_align);
      impl->size = max_size;
      impl->alignment = max_align;
      type->is_incomplete = false;
    }
    break;
  }

  case TYPE_GENERIC_PARAM:
  case TYPE_GENERIC_PACK:
  case TYPE_GENERIC_VALUE:
  case TYPE_PACK_INDEX:
  case TYPE_WILDCARD:
    /* These are compile-time only types, no runtime representation */
    impl->size = 0;
    impl->alignment = 0;
    type->is_incomplete = false;
    break;
  }
}

static void _layout_compute_impl(semantic_type_t type, size_t ptr_size) {
  type_layout_compute(type, ptr_size);
}
