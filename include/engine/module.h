#ifndef _H_CUBEC_ENGINE_MODULE_
#define _H_CUBEC_ENGINE_MODULE_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/map.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_module_t *cubec_module_t;
struct _cubec_module_t {
  char *dirname;
  char *filename;
  char *source;
  cubec_ast_node_t node;
  cubec_map_t exports;
};
cubec_module_t cubec_create_module(cubec_allocator_t allocator,
                                   const char *dirname, const char *filename,
                                   const char *source, cubec_ast_node_t node);

#ifdef __cplusplus
}
#endif
#endif