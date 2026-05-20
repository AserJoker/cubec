#ifndef _H_ENGINE_MODULE_
#define _H_ENGINE_MODULE_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/list.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _module_t *module_t;
struct _module_t {
  char *filename;
  char *dirname;
  value_t value;
  bool master;
  ast_doc_t doc;
  list_t errors;
};
module_t create_module(allocator_t allocator, const char *filename,
                       value_t value, ast_doc_t doc);
#ifdef __cplusplus
}
#endif
#endif