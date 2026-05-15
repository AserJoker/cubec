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
#include "ast/statement_function.h"
#include "ast/statement_if.h"
#include "ast/statement_import.h"
#include "ast/statement_return.h"
#include "ast/statement_struct.h"
#include "ast/statement_switch.h"
#include "ast/statement_test.h"
#include "ast/statement_while.h"
#include "reader/token.h"

ast_node_t read_statement(allocator_t allocator, token_stream_t stream) {
  ast_node_t node = read_statement_empty(allocator, stream);
  if (node) {
    return node;
  }
  node = read_statement_block(allocator, stream);
  if (node) {
    return node;
  }
  node = read_statement_test(allocator, stream);
  if (node) {
    return node;
  }
  node = read_statement_while(allocator, stream);
  if (node) {
    return node;
  }
  node = read_statement_switch(allocator, stream);
  if (node) {
    return node;
  }
  node = read_statement_do_while(allocator, stream);
  if (node) {
    return node;
  }
  node = read_statement_if(allocator, stream);
  if (node) {
    return node;
  }
  node = read_statement_for(allocator, stream);
  if (node) {
    return node;
  }
  node = read_statement_defer(allocator, stream);
  if (node) {
    return node;
  }
  node = read_statement_return(allocator, stream);
  if (node) {
    return node;
  }
  node = read_statement_break(allocator, stream);
  if (node) {
    return node;
  }
  node = read_statement_continue(allocator, stream);
  if (node) {
    return node;
  }
  node = read_statement_function(allocator, stream);
  if (node) {
    return node;
  }
  node = read_statement_struct(allocator, stream);
  if (node) {
    return node;
  }
  node = read_statement_enum(allocator, stream);
  if (node) {
    return node;
  }
  node = read_statement_import(allocator, stream);
  if (node) {
    return node;
  }
  node = read_statement_declaration(allocator, stream);
  if (node) {
    return node;
  }
  node = read_statement_expression(allocator, stream);
  if (node) {
    return node;
  }
  return node;
}