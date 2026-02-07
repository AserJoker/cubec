#ifndef _H_CUBEC_ASTWRITER_NODE_
#define _H_CUBEC_ASTWRITER_NODE_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/value.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t cubec_write_ast_node(cubec_ast_node_t self,
                                   cubec_allocator_t allocator);
#ifdef __cplusplus
}
#endif
#endif