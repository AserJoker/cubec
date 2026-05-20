#ifndef _H_ENGINE_MODULE_
#define _H_ENGINE_MODULE_
#include "core/allocator.h"
#include "engine/type.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _module_t *module_t;
struct _module_t {
  char *filename;
  char *dirname;
  type_t stru;
  bool master;
};
module_t create_module(allocator_t allocator, type_t stru,
                       const char *filename);
#ifdef __cplusplus
}
#endif
#endif