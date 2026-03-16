#ifndef _H_CUBEC_AST_EXPRESSION_TEMPLATE_GENERATOR_
#define _H_CUBEC_AST_EXPRESSION_TEMPLATE_GENERATOR_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/position.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _cubec_ast_expression_template_generator_t {
  struct _cubec_ast_node_t super;
  cubec_ast_node_t temp;
  cubec_ast_node_t args;
} *cubec_ast_expression_template_generator_t;

cubec_ast_expression_template_generator_t
cubec_create_ast_expression_template_generator(cubec_allocator_t allocator);

cubec_ast_node_t cubec_read_ast_expression_template_generator(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end);

#ifdef __cplusplus
}
#endif
#endif