#include "ast/struct_declarator.h"
#include "ast/argument.h"
#include "ast/argument_rest.h"
#include "ast/decorator.h"
#include "ast/expression_condition.h"
#include "ast/expression_spread.h"
#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/statement_declaration.h"
#include "ast/statement_enum.h"
#include "ast/statement_function.h"
#include "ast/statement_struct.h"
#include "ast/struct_field.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"
#include "reader/token.h"

ast_node_t read_struct_declarator(allocator_t allocator,
                                  token_stream_t stream) {
  // TODO:
}