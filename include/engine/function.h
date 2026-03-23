#ifndef _H_CUBEC_ENGINE_FUNCTION_
#define _H_CUBEC_ENGINE_FUNCTION_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/array.h"
#include "engine/type.h"
#include "engine/value.h"
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_function_meta_t *cubec_function_meta_t;
struct _cubec_function_meta_t {
  cubec_type_t type;
  cubec_array_t args;
  bool is_variadic;
};
cubec_function_meta_t cubec_create_function_meta(cubec_allocator_t allocator,
                                                 cubec_type_t type,
                                                 size_t num_args,
                                                 cubec_type_t *args,
                                                 bool is_variadic);
typedef enum _cubec_function_kind_t {
  CUBEC_FUNCTION_NATIVE,
  CUBEC_FUNCTION_RUNTIME,
  CUBEC_FUNCTION_COMPTIME,
} cubec_function_kind_t;
struct _cubec_context_t;
typedef cubec_value_t (*cubec_native_handle_t)(struct _cubec_context_t *ctx,
                                               size_t argc,
                                               cubec_value_t *argv);
typedef struct _cubec_function_desc_t *cubec_function_desc_t;
struct _cubec_function_desc_t {
  char *name;
  cubec_function_kind_t kind;
  union {
    cubec_ast_node_t node;
    cubec_native_handle_t native;
    void *ptr;
  };
};
cubec_function_desc_t cubec_create_function_desc(cubec_allocator_t allocator,
                                                 cubec_function_kind_t kind,
                                                 void *ptr, const char *name);
#ifdef __cplusplus
}
#endif
#endif