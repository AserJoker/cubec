#ifndef _H_ENGINE_STRUCT_
#define _H_ENGINE_STRUCT_
#include "core/array.h"
#include "core/hash_map.h"
#include "engine/context.h"
#include "engine/type.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _struct_field_t *struct_field_t;
struct _struct_field_t {
  size_t offset;
  char *name;
  type_t type;
  bool pub;
  bool mut;
};
typedef struct _struct_meta_t *struct_meta_t;
struct _struct_meta_t {
  array_t fields;
  hash_map_t methods;
  hash_map_t attributes;
  bool packed;
};
typedef struct _struct_attribute_t *struct_attribute_t;
struct _struct_attribute_t {
  bool pub;
  bool mut;
  value_t value;
};
type_t create_struct_type(context_t ctx, const char *id, const char *name);
value_t struct_type_add_field(context_t ctx, type_t stru, const char *name,
                              type_t type, bool pub, bool mut);
array_t struct_type_get_fields(type_t stru);
struct_field_t struct_type_get_field(type_t stru, const char *name);
value_t struct_type_add_method(context_t ctx, type_t stru, const char *name,
                               value_t value, bool pub);
hash_map_t struct_type_get_methods(type_t stru);
struct_attribute_t struct_type_get_method(type_t stru, const char *name);
value_t struct_type_add_attribute(context_t ctx, type_t stru, const char *name,
                                  value_t value, bool pub, bool mut);
hash_map_t struct_type_get_attributes(type_t stru);
struct_attribute_t struct_type_get_attribute(type_t stru, const char *name);
void struct_type_set_packed(type_t stru);
void struct_type_set_aligned(type_t stru, size_t aligned);

#ifdef __cplusplus
}
#endif
#endif