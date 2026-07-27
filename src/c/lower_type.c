#include "c/c_type.h"
#include "c/mangle.h"
#include "engine/semantic_type.h"
#include "engine/symbol.h"
#include "engine/scope.h"
#include <stdio.h>
#include <string.h>

/**
 * @brief Map a semantic_type_t to a C type string.
 *
 * This is an internal function used by the lowering passes.
 * The module_hash is needed for mangled type names.
 */
c_type_t lower_type(allocator_t allocator, semantic_type_t type,
                     const char *module_hash);

/* Forward declaration for recursive types */
static c_type_t lower_type_impl(allocator_t allocator, semantic_type_t type,
                                  const char *module_hash) {
  if (!type) return c_type_primitive(allocator, "void");

  enum type_kind kind = semantic_type_get_kind(type);
  type_impl_t impl = semantic_type_get_impl(type);

  switch (kind) {
  /* Integer types */
  case TYPE_I8:   return c_type_primitive(allocator, "int8_t");
  case TYPE_I16:  return c_type_primitive(allocator, "int16_t");
  case TYPE_I32:  return c_type_primitive(allocator, "int32_t");
  case TYPE_I64:  return c_type_primitive(allocator, "int64_t");
  case TYPE_U8:   return c_type_primitive(allocator, "uint8_t");
  case TYPE_U16:  return c_type_primitive(allocator, "uint16_t");
  case TYPE_U32:  return c_type_primitive(allocator, "uint32_t");
  case TYPE_U64:  return c_type_primitive(allocator, "uint64_t");

  /* Float types */
  case TYPE_F16:  return c_type_primitive(allocator, "_Float16");
  case TYPE_F32:  return c_type_primitive(allocator, "float");
  case TYPE_F64:  return c_type_primitive(allocator, "double");

  /* Bool */
  case TYPE_BOOL: return c_type_primitive(allocator, "bool");

  /* Void */
  case TYPE_VOID: return c_type_primitive(allocator, "void");

  /* Char */
  case TYPE_CHAR: return c_type_primitive(allocator, "uint8_t");

  /* String — mapped to slice<u8> typedef */
  case TYPE_STRING:
  case TYPE_STR: {
    string_t name = mangle_name(allocator, module_hash, "string");
    c_type_t t = c_type_primitive(allocator, string_get(name));
    allocator_free(allocator, &name);
    return t;
  }

  /* Pointer */
  case TYPE_POINTER: {
    c_type_t base = lower_type_impl(allocator, impl->pointer.pointee, module_hash);
    return c_type_pointer(allocator, base);
  }

  /* Slice — typedef name */
  case TYPE_SLICE: {
    /* Generate: m3a7_slice_T — use element type's C name as suffix */
    c_type_t elem_type = lower_type_impl(allocator, impl->slice.element, module_hash);
    /* Build slice typedef name from element's left part */
    char buf[128];
    snprintf(buf, sizeof(buf), "%s_slice_%s", module_hash,
             string_get(elem_type->left));
    /* Replace spaces and * in name */
    string_t name = allocator_create(allocator, &g_string_type,
                                      &(string_init_t){.str = buf});
    c_type_t t = c_type_primitive(allocator, string_get(name));
    c_type_dispose(allocator, &elem_type);
    allocator_free(allocator, &name);
    return t;
  }

  /* Array */
  case TYPE_ARRAY: {
    c_type_t base = lower_type_impl(allocator, impl->array.element, module_hash);
    return c_type_array(allocator, base, impl->array.length);
  }

  /* Struct / Union / CUnion — mangled typedef name */
  case TYPE_STRUCT:
  case TYPE_UNION:
  case TYPE_CUNION: {
    const char *type_name = semantic_type_get_name(type);
    if (type_name) {
      string_t mangled = mangle_name(allocator, module_hash, type_name);
      c_type_t t = c_type_primitive(allocator, string_get(mangled));
      allocator_free(allocator, &mangled);
      return t;
    }
    /* Anonymous — use hash-based name */
    char buf[64];
    snprintf(buf, sizeof(buf), "%s_anon_%zx", module_hash, impl->hash & 0xFFFF);
    return c_type_primitive(allocator, buf);
  }

  /* Enum — mangled typedef name */
  case TYPE_ENUM: {
    const char *type_name = semantic_type_get_name(type);
    if (type_name) {
      string_t mangled = mangle_name(allocator, module_hash, type_name);
      c_type_t t = c_type_primitive(allocator, string_get(mangled));
      allocator_free(allocator, &mangled);
      return t;
    }
    return c_type_primitive(allocator, "int32_t");
  }

  /* Function type — function pointer */
  case TYPE_FUNCTION: {
    c_type_t ret = lower_type_impl(allocator, impl->function.return_type, module_hash);
    vec_t param_types = impl->function.params;
    vec_t c_param_types = allocator_create(allocator, &g_vec_type,
                                            &(vec_init_t){.auto_dispose = false});
    size_t param_count = param_types ? vec_get_size(param_types) : 0;
    for (size_t i = 0; i < param_count; i++) {
      semantic_type_t param = vec_get(param_types, i);
      c_type_t c_param = lower_type_impl(allocator, param, module_hash);
      vec_push(c_param_types, c_param);
    }
    c_type_t t = c_type_function_ptr(allocator, ret, c_param_types,
                                       impl->function.is_variadic);
    /* Dispose param types */
    for (size_t i = 0; i < param_count; i++) {
      c_type_t p = vec_get(c_param_types, i);
      c_type_dispose(allocator, &p);
    }
    allocator_free(allocator, &c_param_types);
    c_type_dispose(allocator, &ret);
    return t;
  }

  /* Qualifier (const/volatile) */
  case TYPE_QUALIFIER: {
    c_type_t base = lower_type_impl(allocator, impl->qualifier.base, module_hash);
    if (impl->qualifier.is_const) base = c_type_const(allocator, base);
    if (impl->qualifier.is_volatile) base = c_type_volatile(allocator, base);
    return base;
  }

  /* Generic instance — use the instantiated type */
  case TYPE_GENERIC_INSTANCE: {
    /* The generic instance has resolved fields, use its name if available */
    const char *type_name = semantic_type_get_name(type);
    if (type_name) {
      string_t mangled = mangle_name(allocator, module_hash, type_name);
      c_type_t t = c_type_primitive(allocator, string_get(mangled));
      allocator_free(allocator, &mangled);
      return t;
    }
    /* Fallback: use template's name */
    return lower_type_impl(allocator, impl->generic_instance.generic_template,
                            module_hash);
  }

  /* Tuple — mapped to anonymous struct typedef */
  case TYPE_TUPLE: {
    char buf[64];
    snprintf(buf, sizeof(buf), "%s_tuple_%zx", module_hash, impl->hash & 0xFFFF);
    return c_type_primitive(allocator, buf);
  }

  /* Nil, Opaque, Wildcard — void* or placeholder */
  case TYPE_NIL:     return c_type_primitive(allocator, "void*");
  case TYPE_OPAQUE:  return c_type_primitive(allocator, "void*");
  case TYPE_WILDCARD: return c_type_primitive(allocator, "void*");

  /* Generic param — shouldn't appear in lowered code (monomorphized) */
  case TYPE_GENERIC_PARAM:
  case TYPE_GENERIC_PACK:
  case TYPE_GENERIC_VALUE:
  case TYPE_PACK_INDEX:
    return c_type_primitive(allocator, "void*");

  /* Interface — no C output */
  case TYPE_INTERFACE: return c_type_primitive(allocator, "void");

  /* Type-of-type */
  case TYPE_TYPE: return lower_type_impl(allocator, impl->type_of.inner, module_hash);

  /* Module — shouldn't appear as a type */
  case TYPE_MODULE: return c_type_primitive(allocator, "void*");

  /* Error */
  case TYPE_ERROR: return c_type_primitive(allocator, "void");
  }

  return c_type_primitive(allocator, "void");
}

c_type_t lower_type(allocator_t allocator, semantic_type_t type,
                     const char *module_hash) {
  return lower_type_impl(allocator, type, module_hash);
}
