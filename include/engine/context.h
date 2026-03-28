#ifndef _H_CUBEC_ENGINE_CONTEXT_
#define _H_CUBEC_ENGINE_CONTEXT_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/map.h"
#include "engine/function.h"
#include "engine/module.h"
#include "engine/scope.h"
#include "engine/type.h"
#include "engine/value.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_context_t *cubec_context_t;
typedef enum _cubec_eval_mode_t {
  CUBEC_EVAL_RUNTIME,
  CUBEC_EVAL_COMPTIME,
  CUBEC_EVAL_TEST,
} cubec_eval_mode_t;
struct _cubec_context_t {
  cubec_allocator_t allocator;
  cubec_scope_t root;
  cubec_scope_t current;
  cubec_map_t modules;
  cubec_array_t strings;
  cubec_module_t module;
  cubec_module_t global;

  cubec_type_t type_void;
  cubec_type_t type_int8;
  cubec_type_t type_int16;
  cubec_type_t type_int32;
  cubec_type_t type_int64;
  cubec_type_t type_uint8;
  cubec_type_t type_uint16;
  cubec_type_t type_uint32;
  cubec_type_t type_uint64;
  cubec_type_t type_float32;
  cubec_type_t type_float64;
  cubec_type_t type_boolean;
  cubec_type_t type_str;
  cubec_type_t type_opaque;

  cubec_type_t type_error;
  cubec_type_t type_type;

  cubec_value_t value_undefined;

  cubec_eval_mode_t eval_mode;
  size_t num_visits;
  cubec_visit_ast_fn_t *visits;
};
cubec_context_t cubec_create_context(cubec_allocator_t allocator);
cubec_module_t cubec_context_get_module(cubec_context_t self, const char *name);
cubec_value_t cubec_context_load_module(cubec_context_t self, const char *name);
cubec_module_t cubec_context_set_module(cubec_context_t self,
                                        cubec_module_t module);
cubec_type_t cubec_context_create_type(cubec_context_t self,
                                       cubec_type_kind_t kind, size_t size,
                                       void *meta, const char *name);
void cubec_context_push_scope(cubec_context_t self);
void cubec_context_pop_scope(cubec_context_t self);
cubec_type_t cubec_context_create_ptr_type(cubec_context_t self,
                                           cubec_type_t type, bool is_mutable,
                                           bool is_volatile);
cubec_type_t cubec_context_create_ptr_array_type(cubec_context_t self,
                                                 cubec_type_t type,
                                                 bool is_mutable,
                                                 bool is_volatile);
cubec_type_t cubec_context_create_array_type(cubec_context_t self,
                                             cubec_type_t type, size_t length);
cubec_type_t cubec_context_create_struct_type(cubec_context_t self,
                                              size_t align, const char *name);
cubec_type_t cubec_context_create_union_type(cubec_context_t self, size_t align,
                                             const char *name);
cubec_value_t cubec_context_add_struct_field(cubec_context_t self,
                                             cubec_type_t stru,
                                             const char *name,
                                             cubec_type_t type);
cubec_value_t cubec_context_add_struct_attribute(cubec_context_t self,
                                                 cubec_type_t stru,
                                                 const char *name,
                                                 cubec_value_t value);
cubec_type_t cubec_context_create_result_type(cubec_context_t self,
                                              cubec_type_t type,
                                              cubec_type_t etype);
cubec_type_t cubec_context_create_function_type(cubec_context_t self,
                                                cubec_type_t type,
                                                size_t num_args,
                                                cubec_type_t *args,
                                                bool is_variadic);
cubec_value_t cubec_context_create_value(cubec_context_t self,
                                         cubec_type_t type, bool is_mutable,
                                         const void *data, const char *name);
cubec_value_t cubec_context_create_int8(cubec_context_t self, int8_t value,
                                        bool is_mutable, const char *name);
cubec_value_t cubec_context_create_int16(cubec_context_t self, int16_t value,
                                         bool is_mutable, const char *name);
cubec_value_t cubec_context_create_int32(cubec_context_t self, int32_t value,
                                         bool is_mutable, const char *name);
cubec_value_t cubec_context_create_int64(cubec_context_t self, int64_t value,
                                         bool is_mutable, const char *name);
cubec_value_t cubec_context_create_uint8(cubec_context_t self, uint8_t value,
                                         bool is_mutable, const char *name);
cubec_value_t cubec_context_create_uint16(cubec_context_t self, uint16_t value,
                                          bool is_mutable, const char *name);
cubec_value_t cubec_context_create_uint32(cubec_context_t self, uint32_t value,
                                          bool is_mutable, const char *name);
cubec_value_t cubec_context_create_uint64(cubec_context_t self, uint64_t value,
                                          bool is_mutable, const char *name);
cubec_value_t cubec_context_create_float32(cubec_context_t self, float value,
                                           bool is_mutable, const char *name);
cubec_value_t cubec_context_create_float64(cubec_context_t self, double value,
                                           bool is_mutable, const char *name);
cubec_value_t cubec_context_create_boolean(cubec_context_t self, bool value,
                                           bool is_mutable, const char *name);
cubec_value_t cubec_context_create_str(cubec_context_t self, const char *str,
                                       bool is_mutable, const char *name);
cubec_value_t cubec_context_create_opaque(cubec_context_t self, void *data,
                                          bool is_mutable, const char *name);
cubec_value_t cubec_context_create_ptr(cubec_context_t self,
                                       cubec_value_t value, bool is_mutable,
                                       const char *name);
cubec_value_t cubec_context_create_ptr_array(cubec_context_t self,
                                             cubec_value_t value,
                                             bool is_mutable, const char *name);
cubec_value_t cubec_context_create_array(cubec_context_t self,
                                         cubec_type_t type, size_t length,
                                         bool is_mutable, const char *name);
cubec_value_t cubec_context_create_struct(cubec_context_t self,
                                          cubec_type_t type, bool is_mutable,
                                          const char *name);
cubec_value_t cubec_context_create_union(cubec_context_t self,
                                         cubec_type_t type, bool is_mutable,
                                         const char *name);
cubec_value_t cubec_context_create_error(cubec_context_t self, const char *fmt,
                                         ...);
cubec_value_t cubec_context_create_compile_error(cubec_context_t self,
                                                 cubec_ast_node_t node,
                                                 const char *filename,
                                                 const char *fmt, ...);
cubec_value_t cubec_context_convert_compile_error(cubec_context_t self,
                                                  cubec_ast_node_t node,
                                                  const char *filename,
                                                  cubec_value_t error);
cubec_value_t cubec_context_create_result(cubec_context_t self,
                                          cubec_type_t type,
                                          cubec_value_t value,
                                          cubec_value_t error, bool is_mutable,
                                          const char *name);
cubec_value_t cubec_context_create_function(cubec_context_t self,
                                            cubec_type_t type,
                                            cubec_ast_node_t node,
                                            bool is_mutable, const char *name);
cubec_value_t cubec_context_create_native(cubec_context_t self,
                                          cubec_type_t type,
                                          cubec_native_handle_t native,
                                          bool is_mutable, const char *name);
cubec_value_t cubec_context_create_type_value(cubec_context_t self,
                                              cubec_type_t type,
                                              bool is_mutable,
                                              const char *name);
bool cubec_context_check_type(cubec_context_t self, cubec_type_t dst,
                              cubec_type_t src);
char *cubec_context_type_to_string(cubec_context_t self, cubec_type_t type);
cubec_value_t cubec_context_set_index(cubec_context_t self, cubec_value_t arr,
                                      size_t idx, cubec_value_t value);
cubec_value_t cubec_context_get_index(cubec_context_t self, cubec_value_t arr,
                                      size_t idx);
cubec_value_t cubec_context_set_field(cubec_context_t self, cubec_value_t obj,
                                      const char *field, cubec_value_t value);
cubec_value_t cubec_context_get_field(cubec_context_t self, cubec_value_t obj,
                                      const char *field);
cubec_value_t cubec_context_call(cubec_context_t self, cubec_value_t func,
                                 size_t argc, cubec_value_t *argv);
cubec_value_t cubec_context_load_value(cubec_context_t self, const char *name);

cubec_value_t cubec_context_inc_value(cubec_context_t self,
                                      cubec_value_t value);
cubec_value_t cubec_context_dec_value(cubec_context_t self,
                                      cubec_value_t value);
int64_t cubec_context_value_to_int64(cubec_context_t self, cubec_value_t value);
uint64_t cubec_context_value_to_uint64(cubec_context_t self,
                                       cubec_value_t value);
cubec_value_t cubec_context_read_ptr(cubec_context_t ctx, cubec_value_t value);
cubec_value_t cubec_context_write_ptr(cubec_context_t ctx, cubec_value_t ptr,
                                      cubec_value_t value);
cubec_value_t cubec_context_to_boolean(cubec_context_t ctx,
                                       cubec_value_t value);
cubec_value_t cubec_context_plus(cubec_context_t ctx, cubec_value_t value);
cubec_value_t cubec_context_negtive(cubec_context_t ctx, cubec_value_t value);
cubec_value_t cubec_context_typeof(cubec_context_t ctx, cubec_value_t value);
cubec_value_t cubec_context_bitwise_not(cubec_context_t ctx,
                                        cubec_value_t value);
cubec_value_t cubec_context_logical_not(cubec_context_t ctx,
                                        cubec_value_t value);
#ifdef __cplusplus
}
#endif
#endif