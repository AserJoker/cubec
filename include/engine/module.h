#ifndef _H_ENGINE_MODULE_
#define _H_ENGINE_MODULE_
#include "ast/node.h"
#include "core/allocator.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _module_t *module_t;
module_t create_module(allocator_t allocator, value_t value, ast_node_t node,
                       const char *filename);
value_t module_get_value(module_t self);
const char *module_get_filename(module_t self);
const char *module_get_dirname(module_t self);
#ifdef __cplusplus
}
#endif
#endif