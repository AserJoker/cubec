#ifndef _H_CUBEC_ENGINE_MODULE_
#define _H_CUBEC_ENGINE_MODULE_
#include "ast/node.h"
#include "core/allocator.h"
#include "engine/value.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _module_t *module_t;
module_t create_module(allocator_t allocator, ast_node_t node,
                       const char *filename, char *source, value_t value);
const char *module_get_filename(module_t self);
const char *module_get_dirname(module_t self);
ast_node_t module_get_node(module_t self);
value_t module_get_value(module_t self);
#ifdef __cplusplus
}
#endif
#endif