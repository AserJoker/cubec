#include "ast/expression_condition.h"
#include "ast/expression_binary.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "reader/token.h"

ast_node_t read_expression_condition(allocator_t allocator,
                                     token_stream_t stream) {
  return read_expression_logical_or(allocator, stream);
}