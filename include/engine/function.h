#ifndef _H_CUBEC_ENGINE_FUNCTION_
#define _H_CUBEC_ENGINE_FUNCTION_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/array.h"
#include "engine/type.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_function_meta_t *cubec_function_meta_t;
struct _cubec_function_meta_t {
  cubec_array_t args;
  cubec_type_t type;
  bool variadic;
};
struct _cubec_context_t;
typedef cubec_value_t (*cubec_native_handle_fn_t)(struct _cubec_context_t *ctx,
                                                  size_t argc,
                                                  cubec_value_t *argv);
cubec_function_meta_t cubec_create_function_meta(cubec_allocator_t allocator,
                                                 cubec_array_t args,
                                                 cubec_type_t type,
                                                 bool variadic);
typedef struct _cubec_function_data_t *cubec_function_data_t;
struct _cubec_function_data_t {
  void *pfunc;
};
typedef enum _cubec_function_kind_t {
  CUBEC_FUNCTION_COMPTIME,
  CUBEC_FUNCTION_NATIVE,
  CUBEC_FUNCTION_RUNTIME,
} cubec_function_kind_t;
typedef struct _cubec_function_desc_t *cubec_function_desc_t;
struct _cubec_function_desc_t {
  union {
    cubec_ast_node_t node;
    cubec_native_handle_fn_t handle;
  };
  cubec_function_kind_t kind;
};
cubec_function_desc_t
cubec_create_comptime_function_desc(cubec_allocator_t allocator,
                                    cubec_ast_node_t node);
cubec_function_desc_t
cubec_create_runtime_function_desc(cubec_allocator_t allocator);
cubec_function_desc_t
cubec_create_native_function_desc(cubec_allocator_t allocator,
                                  cubec_native_handle_fn_t handle);
#ifdef __cplusplus
}
#endif
#endif