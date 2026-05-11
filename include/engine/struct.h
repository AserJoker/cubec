#ifndef _H_ENGINE_STRUCT_
#define _H_ENGINE_STRUCT_
#include "core/allocator.h"
#include "core/array.h"
#include "engine/context.h"
#include "engine/type.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _struct_field_t *struct_field_t;
struct _struct_field_t {
  char *name;
  size_t offset;
  type_t type;
  bool mut;
  bool pub;
};
typedef struct _struct_attribute_t *struct_attribute_t;
struct _struct_attribute_t {
  char *name;
  value_t value;
  bool pub;
  bool comptime;
};
type_t create_struct_type(context_t ctx, const char *name, size_t align);
void struct_type_lock_align(type_t self);
void struct_type_packed(type_t self);
void struct_type_add_field(type_t self, allocator_t allocator, const char *name,
                           type_t type, bool mut, bool pub);
void struct_type_add_attribute(type_t self, allocator_t allocator,
                               const char *name, value_t value, bool pub,
                               bool comptime);
void struct_type_seal(context_t ctx, type_t self);
array_t struct_type_get_fields(type_t self);
struct_field_t struct_type_get_field(type_t self, const char *name);
void struct_type_remove_field(type_t self, const char *name);
array_t struct_type_get_attributes(type_t self);
struct_attribute_t struct_type_get_attribute(type_t self, const char *name);
void struct_type_remove_attribute(type_t self, const char *name);
bool struct_type_is_packed(type_t self);
bool struct_type_is_aligned(type_t self);
#ifdef __cplusplus
}
#endif
#endif