#include "ast/expression.h"
#include "ast/expression_assigment.h"
#include "ast/expression_comma.h"
#include "ast/node.h"

cubec_ast_node_t cubec_read_ast_expression1(cubec_allocator_t allocator,
                                            cubec_position_t *position,
                                            const char *end) {
  cubec_ast_node_t node =
      cubec_read_ast_expression_comma(allocator, position, end);
  if (node) {
    return node;
  }
  return cubec_read_ast_expression2(allocator, position, end);
}

cubec_ast_node_t cubec_read_ast_expression2(cubec_allocator_t allocator,
                                            cubec_position_t *position,
                                            const char *end) {
  cubec_ast_node_t node =
      cubec_read_ast_expression_assigment(allocator, position, end);
  if (node) {
    return node;
  }
  return cubec_read_ast_expression3(allocator, position, end);
}

cubec_ast_node_t cubec_read_ast_expression3(cubec_allocator_t allocator,
                                            cubec_position_t *position,
                                            const char *end);

cubec_ast_node_t cubec_read_ast_expression4(cubec_allocator_t allocator,
                                            cubec_position_t *position,
                                            const char *end);
cubec_ast_node_t cubec_read_ast_expression5(cubec_allocator_t allocator,
                                            cubec_position_t *position,
                                            const char *end);
cubec_ast_node_t cubec_read_ast_expression6(cubec_allocator_t allocator,
                                            cubec_position_t *position,
                                            const char *end);
cubec_ast_node_t cubec_read_ast_expression7(cubec_allocator_t allocator,
                                            cubec_position_t *position,
                                            const char *end);
cubec_ast_node_t cubec_read_ast_expression8(cubec_allocator_t allocator,
                                            cubec_position_t *position,
                                            const char *end);
cubec_ast_node_t cubec_read_ast_expression9(cubec_allocator_t allocator,
                                            cubec_position_t *position,
                                            const char *end);
cubec_ast_node_t cubec_read_ast_expression10(cubec_allocator_t allocator,
                                             cubec_position_t *position,
                                             const char *end);
cubec_ast_node_t cubec_read_ast_expression11(cubec_allocator_t allocator,
                                             cubec_position_t *position,
                                             const char *end);
cubec_ast_node_t cubec_read_ast_expression12(cubec_allocator_t allocator,
                                             cubec_position_t *position,
                                             const char *end);
cubec_ast_node_t cubec_read_ast_expression13(cubec_allocator_t allocator,
                                             cubec_position_t *position,
                                             const char *end);
cubec_ast_node_t cubec_read_ast_expression14(cubec_allocator_t allocator,
                                             cubec_position_t *position,
                                             const char *end);
cubec_ast_node_t cubec_read_ast_expression15(cubec_allocator_t allocator,
                                             cubec_position_t *position,
                                             const char *end);
cubec_ast_node_t cubec_read_ast_expression16(cubec_allocator_t allocator,
                                             cubec_position_t *position,
                                             const char *end);
cubec_ast_node_t cubec_read_ast_expression17(cubec_allocator_t allocator,
                                             cubec_position_t *position,
                                             const char *end);
cubec_ast_node_t cubec_read_ast_expression18(cubec_allocator_t allocator,
                                             cubec_position_t *position,
                                             const char *end);
cubec_ast_node_t cubec_read_ast_expression19(cubec_allocator_t allocator,
                                             cubec_position_t *position,
                                             const char *end);