#ifndef _H_CUBEC_ENGINE_CONTEXT_
#define _H_CUBEC_ENGINE_CONTEXT_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/list.h"
#include "engine/function.h"
#include "engine/scope.h"
#include "engine/type.h"
#include "engine/value.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_context_t *cubec_context_t;
struct _cubec_context_t {
  cubec_allocator_t allocator;
  cubec_scope_t root;
  cubec_scope_t current;
  cubec_list_t types;
  struct {
    cubec_type_t self_type;
    cubec_type_t void_type;
    cubec_type_t undefined_type;
    cubec_type_t int8_type;
    cubec_type_t int16_type;
    cubec_type_t int32_type;
    cubec_type_t int64_type;
    cubec_type_t uint8_type;
    cubec_type_t uint16_type;
    cubec_type_t uint32_type;
    cubec_type_t uint64_type;
    cubec_type_t float32_type;
    cubec_type_t float64_type;
    cubec_type_t boolean_type;
    cubec_type_t str_type;
    cubec_type_t opaque_type;
    cubec_type_t error_type;
  } named_types;
  struct {
  } constants;
  cubec_list_t strings;
  cubec_list_t functions;
};
cubec_context_t cubec_create_context(cubec_allocator_t allocator);

void cubec_context_push(cubec_context_t self);

void cubec_context_pop(cubec_context_t self);

cubec_type_t cubec_context_create_type(cubec_context_t self,
                                       cubec_type_kind_t kind, size_t size,
                                       const char *name, void *meta);

cubec_type_t cubec_context_load_type(cubec_context_t self, const char *name);

cubec_type_t cubec_context_store_type(cubec_context_t self, const char *name,
                                      cubec_type_t type);

cubec_type_t cubec_context_get_ptr_type(cubec_context_t self, cubec_type_t src);

cubec_type_t cubec_context_get_ptr_array_type(cubec_context_t self,
                                              cubec_type_t src);

cubec_type_t cubec_context_get_ref_type(cubec_context_t self, cubec_type_t src);

cubec_type_t cubec_context_create_union_type(cubec_context_t self,
                                             const cubec_array_t types);

cubec_type_t cubec_context_create_function_type(cubec_context_t self,
                                                size_t argc, cubec_type_t *argv,
                                                cubec_type_t type,
                                                bool variadic);

cubec_type_t cubec_context_create_array_type(cubec_context_t self,
                                             cubec_type_t type, size_t length);

cubec_type_t cubec_context_create_struct_type(cubec_context_t self);

void cubec_context_add_struct_field(cubec_context_t self,
                                    cubec_type_t struct_type, const char *field,
                                    cubec_type_t type);
void cubec_context_add_struct_attribute(cubec_context_t self,
                                        cubec_type_t struct_type,
                                        const char *field, cubec_value_t value);

cubec_type_t cubec_context_create_enum_type(cubec_context_t self,
                                            cubec_type_t type);

void cubec_context_add_enum_option(cubec_context_t self, cubec_type_t enum_type,
                                   const char *name, cubec_value_t value);

cubec_value_t cubec_context_create_enum_value(cubec_context_t self,
                                              cubec_type_t type,
                                              const char *option,const char *name);

cubec_value_t cubec_context_create_union_value(cubec_context_t self,
                                               cubec_type_t type,
                                               cubec_value_t value,
                                               const char *name);

cubec_value_t cubec_context_unwrap_union(cubec_context_t self,
                                         cubec_value_t value);

cubec_value_t cubec_context_get_index(cubec_context_t self, cubec_value_t value,
                                      size_t idx);

cubec_value_t cubec_context_get_field(cubec_context_t self, cubec_value_t value,
                                      const char *field);

cubec_value_t cubec_context_create_comptime_function(cubec_context_t self,
                                                     cubec_type_t type,
                                                     cubec_ast_node_t node,
                                                     const char *name);

cubec_value_t
cubec_context_create_native_function(cubec_context_t self, cubec_type_t type,
                                     cubec_native_handle_fn_t handle,
                                     const char *name);

cubec_value_t cubec_context_create_error(cubec_context_t self,
                                         const char *message, const char *name);

cubec_value_t cubec_context_create_ref(cubec_context_t self, cubec_value_t src,
                                       const char *name);

cubec_value_t cubec_context_create_ptr(cubec_context_t self, cubec_value_t src,
                                       const char *name);

cubec_value_t cubec_context_create_ptr_array(cubec_context_t self,
                                             cubec_value_t src,
                                             const char *name);

cubec_value_t cubec_context_create_value(cubec_context_t self,
                                         cubec_type_t type, const void *init,
                                         const char *name);
cubec_value_t cubec_context_create_int8(cubec_context_t self, int8_t value,
                                        const char *name);
cubec_value_t cubec_context_create_int16(cubec_context_t self, int16_t value,
                                         const char *name);
cubec_value_t cubec_context_create_int32(cubec_context_t self, int32_t value,
                                         const char *name);
cubec_value_t cubec_context_create_int64(cubec_context_t self, int64_t value,
                                         const char *name);
cubec_value_t cubec_context_create_uint8(cubec_context_t self, uint8_t value,
                                         const char *name);
cubec_value_t cubec_context_create_uint16(cubec_context_t self, uint16_t value,
                                          const char *name);
cubec_value_t cubec_context_create_uint32(cubec_context_t self, uint32_t value,
                                          const char *name);
cubec_value_t cubec_context_create_uint64(cubec_context_t self, uint64_t value,
                                          const char *name);
cubec_value_t cubec_context_create_float32(cubec_context_t self, float value,
                                           const char *name);
cubec_value_t cubec_context_create_float64(cubec_context_t self, double value,
                                           const char *name);
cubec_value_t cubec_context_create_boolean(cubec_context_t self, bool value,
                                           const char *name);

cubec_value_t cubec_context_create_undefined(cubec_context_t self,
                                             const char *name);

cubec_value_t cubec_context_create_str(cubec_context_t self, const char *value,
                                       const char *name);

cubec_value_t cubec_context_load_value(cubec_context_t self, const char *name);

cubec_value_t cubec_context_eval(cubec_context_t self, const char *filename,
                                 const char *source);

cubec_value_t cubec_context_call(cubec_context_t self, cubec_value_t function,
                                 size_t argc, cubec_value_t *argv);

#ifdef __cplusplus
}
#endif
#endif