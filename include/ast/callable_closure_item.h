#ifndef _H_CALLABLE_CLOSURE_ITEM_
#define _H_CALLABLE_CLOSURE_ITEM_
#include "ast/node.h"
#include "core/allocator.h"
#ifdef __cplusplus
extern "C" {
#endif

ast_node_t read_callable_closure_item(allocator_t allocator,
                                      token_stream_t stream);
#ifdef __cplusplus
}
#endif
#endif