#ifndef _H_CUBEC_PASS_UNWRAP_GROUP_
#define _H_CUBEC_PASS_UNWRAP_GROUP_
#include "ast/node.h"
#include "core/allocator.h"
#ifdef __cpluplus
extern "C" {
#endif
cubec_ast_node_t cubec_pass_unwrap_group(cubec_allocator_t allocator,
                                         cubec_ast_node_t node, void *ctx);
#ifdef __cpluplus
}
#endif
#endif