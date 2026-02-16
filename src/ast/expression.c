#include "ast/expression.h"
#include "ast/expression_assigment.h"
#include "ast/expression_binary.h"
#include "ast/expression_call.h"
#include "ast/expression_comma.h"
#include "ast/expression_compute_member.h"
#include "ast/expression_group.h"
#include "ast/expression_member.h"
#include "ast/expression_template_generator.h"
#include "ast/literal_char.h"
#include "ast/literal_identifier.h"
#include "ast/literal_numeric.h"
#include "ast/literal_string.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"

cubec_ast_node_t cubec_read_ast_expression(cubec_allocator_t allocator,
                                           cubec_position_t *position,
                                           const char *end) {
  return cubec_read_ast_expression1(allocator, position, end);
}
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
                                            const char *end) {
  cubec_ast_node_t node = NULL;
  if (node) {
    return node;
  }
  // TODO: triple
  return cubec_read_ast_expression4(allocator, position, end);
}

cubec_ast_node_t cubec_read_ast_expression4(cubec_allocator_t allocator,
                                            cubec_position_t *position,
                                            const char *end) {
  cubec_ast_node_t node =
      cubec_read_ast_expression_binary_logical_or(allocator, position, end);
  if (node) {
    return node;
  }
  return cubec_read_ast_expression5(allocator, position, end);
}
cubec_ast_node_t cubec_read_ast_expression5(cubec_allocator_t allocator,
                                            cubec_position_t *position,
                                            const char *end) {
  cubec_ast_node_t node =
      cubec_read_ast_expression_binary_logical_and(allocator, position, end);
  if (node) {
    return node;
  }
  return cubec_read_ast_expression6(allocator, position, end);
}
cubec_ast_node_t cubec_read_ast_expression6(cubec_allocator_t allocator,
                                            cubec_position_t *position,
                                            const char *end) {
  cubec_ast_node_t node =
      cubec_read_ast_expression_binary_bitwise_or(allocator, position, end);
  if (node) {
    return node;
  }
  return cubec_read_ast_expression7(allocator, position, end);
}
cubec_ast_node_t cubec_read_ast_expression7(cubec_allocator_t allocator,
                                            cubec_position_t *position,
                                            const char *end) {
  cubec_ast_node_t node =
      cubec_read_ast_expression_binary_bitwise_xor(allocator, position, end);
  if (node) {
    return node;
  }
  return cubec_read_ast_expression8(allocator, position, end);
}
cubec_ast_node_t cubec_read_ast_expression8(cubec_allocator_t allocator,
                                            cubec_position_t *position,
                                            const char *end) {
  cubec_ast_node_t node =
      cubec_read_ast_expression_binary_bitwise_and(allocator, position, end);
  if (node) {
    return node;
  }
  return cubec_read_ast_expression9(allocator, position, end);
}
cubec_ast_node_t cubec_read_ast_expression9(cubec_allocator_t allocator,
                                            cubec_position_t *position,
                                            const char *end) {
  cubec_ast_node_t node =
      cubec_read_ast_expression_binary_equal(allocator, position, end);
  if (node) {
    return node;
  }
  return cubec_read_ast_expression10(allocator, position, end);
}
cubec_ast_node_t cubec_read_ast_expression10(cubec_allocator_t allocator,
                                             cubec_position_t *position,
                                             const char *end) {
  cubec_ast_node_t node =
      cubec_read_ast_expression_binary_relation(allocator, position, end);
  if (node) {
    return node;
  }
  return cubec_read_ast_expression11(allocator, position, end);
}
cubec_ast_node_t cubec_read_ast_expression11(cubec_allocator_t allocator,
                                             cubec_position_t *position,
                                             const char *end) {
  cubec_ast_node_t node =
      cubec_read_ast_expression_binary_bitwise_shift(allocator, position, end);
  if (node) {
    return node;
  }
  return cubec_read_ast_expression12(allocator, position, end);
}
cubec_ast_node_t cubec_read_ast_expression12(cubec_allocator_t allocator,
                                             cubec_position_t *position,
                                             const char *end) {
  cubec_ast_node_t node =
      cubec_read_ast_expression_binary_additive(allocator, position, end);
  if (node) {
    return node;
  }
  return cubec_read_ast_expression13(allocator, position, end);
}
cubec_ast_node_t cubec_read_ast_expression13(cubec_allocator_t allocator,
                                             cubec_position_t *position,
                                             const char *end) {
  cubec_ast_node_t node =
      cubec_read_ast_expression_binary_multiplicative(allocator, position, end);
  if (node) {
    return node;
  }
  return cubec_read_ast_expression14(allocator, position, end);
}
cubec_ast_node_t cubec_read_ast_expression14(cubec_allocator_t allocator,
                                             cubec_position_t *position,
                                             const char *end) {
  return cubec_read_ast_expression15(allocator, position, end);
}
cubec_ast_node_t cubec_read_ast_expression15(cubec_allocator_t allocator,
                                             cubec_position_t *position,
                                             const char *end) {
  cubec_ast_node_t node =
      cubec_read_ast_expression_binary_prefix(allocator, position, end);
  if (node) {
    return node;
  }
  return cubec_read_ast_expression16(allocator, position, end);
}
cubec_ast_node_t cubec_read_ast_expression16(cubec_allocator_t allocator,
                                             cubec_position_t *position,
                                             const char *end) {
  cubec_ast_node_t node =
      cubec_read_ast_expression_binary_postfix(allocator, position, end);
  if (node) {
    return node;
  }
  return cubec_read_ast_expression17(allocator, position, end);
}
cubec_ast_node_t cubec_read_ast_expression17(cubec_allocator_t allocator,
                                             cubec_position_t *position,
                                             const char *end) {
  return cubec_read_ast_expression18(allocator, position, end);
}
cubec_ast_node_t cubec_read_ast_expression18(cubec_allocator_t allocator,
                                             cubec_position_t *position,
                                             const char *end) {
  cubec_ast_node_t node = NULL;
  cubec_position_t current = *position;
  node = cubec_read_ast_expression19(allocator, &current, end);
  if (node) {
    for (;;) {
      cubec_ast_node_t err = cubec_ast_skip_all(allocator, &current, end);
      if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
        return err;
      }
      cubec_ast_node_t next = NULL;
      if (!next) {
        next = cubec_read_ast_expression_member(allocator, &current, end);
      }
      if (!next) {
        next =
            cubec_read_ast_expression_compute_member(allocator, &current, end);
      }
      if (!next) {
        next = cubec_read_ast_expression_call(allocator, &current, end);
      }
      if (next) {
        if (next->type == CUBEC_NODE_TYPE_ERROR) {
          cubec_allocator_free(allocator, node);
          next->loc.begin = *position;
          return next;
        }
        if (next->type == CUBEC_NODE_TYPE_EXPRESSION_MEMBER) {
          cubec_ast_expression_member_t member =
              (cubec_ast_expression_member_t)next;
          member->host = node;
          node = &member->super;
          member->super.loc.begin = *position;
        } else if (next->type == CUBEC_NODE_TYPE_EXPRESSION_COMPUTE_MEMBER) {
          cubec_ast_expression_compute_member_t member =
              (cubec_ast_expression_compute_member_t)next;
          member->host = node;
          node = &member->super;
          member->super.loc.begin = *position;
        } else if (next->type == CUBEC_NODE_TYPE_EXPRESSION_CALL) {
          cubec_ast_expression_call_t call = (cubec_ast_expression_call_t)next;
          call->callee = node;
          node = &call->super;
          call->super.loc.begin = *position;
        }
      } else {
        break;
      }
    }
  }
  *position = current;
  return node;
}
cubec_ast_node_t cubec_read_ast_expression19(cubec_allocator_t allocator,
                                             cubec_position_t *position,
                                             const char *end) {
  cubec_ast_node_t node = NULL;
  // TODO: func
  // TODO: template
  node = cubec_read_ast_expression_group(allocator, position, end);
  if (node) {
    return node;
  }
  node = cubec_read_ast_expression_template_generator(allocator, position, end);
  if (node) {
    return node;
  }
  node = cubec_read_ast_literal_numeric(allocator, position, end);
  if (node) {
    return node;
  }
  node = cubec_read_ast_literal_char(allocator, position, end);
  if (node) {
    return node;
  }
  node = cubec_read_ast_literal_string(allocator, position, end);
  if (node) {
    return node;
  }
  node = cubec_read_ast_literal_identifier(allocator, position, end);
  if (node) {
    return node;
  }
  return NULL;
}