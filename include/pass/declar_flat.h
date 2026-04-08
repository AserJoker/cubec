#ifndef _H_CUBEC_PASS_DECLAR_FLAT_
#define _H_CUBEC_PASS_DECLAR_FLAT_
#include "ast/node.h"
#include "engine/context.h"
#ifdef __cplusplus__
extern "C" {
#endif
cubec_ast_node_t cubec_pass_declar_flat(cubec_allocator_t allocator,
                                        cubec_ast_node_t node,
                                        cubec_context_t ctx);
#ifdef __cplusplus__
}
#endif
#endif