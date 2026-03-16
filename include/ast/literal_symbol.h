#ifndef _H_CUBEC_AST_LITERAL_SYMBOL_
#define _H_CUBEC_AST_LITERAL_SYMBOL_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/position.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _cubec_ast_literal_symbol_t {
  struct _cubec_ast_node_t super;
} *cubec_ast_literal_symbol_t;

cubec_ast_literal_symbol_t
cubec_create_ast_literal_symbol(cubec_allocator_t allocator);
cubec_ast_node_t cubec_read_ast_literal_symbol(cubec_allocator_t allocator,
                                               cubec_position_t *position,
                                               const char *end);

#ifdef __cplusplus
}
#endif
#endif