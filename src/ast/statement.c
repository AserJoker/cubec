#include "ast/statement.h"
#include "ast/node.h"
#include "ast/statement_block.h"
#include "ast/statement_break.h"
#include "ast/statement_continue.h"
#include "ast/statement_declaration.h"
#include "ast/statement_defer.h"
#include "ast/statement_do_while.h"
#include "ast/statement_empty.h"
#include "ast/statement_enum.h"
#include "ast/statement_expression.h"
#include "ast/statement_for.h"
#include "ast/statement_foreach.h"
#include "ast/statement_function.h"
#include "ast/statement_if.h"
#include "ast/statement_import.h"
#include "ast/statement_return.h"
#include "ast/statement_struct.h"
#include "ast/statement_switch.h"
#include "ast/statement_test.h"
#include "ast/statement_while.h"

cubec_ast_node_t cubec_read_ast_statement(cubec_allocator_t allocator,
                                          cubec_position_t *position,
                                          const char *end,
                                          const char *filename) {
  cubec_ast_node_t node =
      cubec_read_ast_statement_empty(allocator, position, end, filename);
  if (node) {
    return node;
  }
  node = cubec_read_ast_statement_block(allocator, position, end, filename);
  if (node) {
    return node;
  }
  node = cubec_read_ast_statement_test(allocator, position, end, filename);
  if (node) {
    return node;
  }
  node = cubec_read_ast_statement_while(allocator, position, end, filename);
  if (node) {
    return node;
  }
  node = cubec_read_ast_statement_switch(allocator, position, end, filename);
  if (node) {
    return node;
  }
  node = cubec_read_ast_statement_do_while(allocator, position, end, filename);
  if (node) {
    return node;
  }
  node = cubec_read_ast_statement_if(allocator, position, end, filename);
  if (node) {
    return node;
  }
  node = cubec_read_ast_statement_for(allocator, position, end, filename);
  if (node) {
    return node;
  }
  node = cubec_read_ast_statement_foreach(allocator, position, end, filename);
  if (node) {
    return node;
  }
  node = cubec_read_ast_statement_defer(allocator, position, end, filename);
  if (node) {
    return node;
  }
  node = cubec_read_ast_statement_return(allocator, position, end, filename);
  if (node) {
    return node;
  }
  node = cubec_read_ast_statement_break(allocator, position, end, filename);
  if (node) {
    return node;
  }
  node = cubec_read_ast_statement_continue(allocator, position, end, filename);
  if (node) {
    return node;
  }
  node = cubec_read_ast_statement_function(allocator, position, end, filename);
  if (node) {
    return node;
  }
  node = cubec_read_ast_statement_struct(allocator, position, end, filename);
  if (node) {
    return node;
  }
  node = cubec_read_ast_statement_enum(allocator, position, end, filename);
  if (node) {
    return node;
  }
  node =
      cubec_read_ast_statement_declaration(allocator, position, end, filename);
  if (node) {
    return node;
  }
  node = cubec_read_ast_statement_import(allocator, position, end, filename);
  if (node) {
    return node;
  }
  node =
      cubec_read_ast_statement_expression(allocator, position, end, filename);
  if (node) {
    return node;
  }
  return node;
}