#ifndef _H_CUBEC_ASTWRITER_STATEMENT_BLOCK_
#define _H_CUBEC_ASTWRITER_STATEMENT_BLOCK_
#include "ast/statement_block.h"
#include "core/any.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_any_t cubec_write_ast_statement_block(cubec_allocator_t allocator,
                                            cubec_ast_statement_block_t self);
#ifdef __cplusplus
}
#endif
#endif