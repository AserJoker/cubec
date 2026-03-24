#ifndef _H_CUBEC_ENGINE_MODULE_
#define _H_CUBEC_ENGINE_MODULE_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/string.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_module_t *cubec_module_t;
struct _cubec_module_t {
  cubec_array_t types;
  cubec_array_t functions;
  cubec_map_t variables;
  cubec_map_t dependences;
  char *filename;
  char *dirname;
  cubec_string_t data;
  cubec_ast_node_t node;
};
cubec_module_t cubec_create_module(cubec_allocator_t allocator,
                                   const char *filename);
#ifdef __cplusplus
}
#endif
#endif