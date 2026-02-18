#include "astwriter/node.h"
#include "ast/enum_declarator.h"
#include "ast/enum_field.h"
#include "ast/expression_assigment.h"
#include "ast/expression_binary.h"
#include "ast/expression_call.h"
#include "ast/expression_comma.h"
#include "ast/expression_compute_member.h"
#include "ast/expression_group.h"
#include "ast/expression_slice.h"
#include "ast/expression_spread.h"
#include "ast/expression_template_generator.h"
#include "ast/function_argument.h"
#include "ast/function_body.h"
#include "ast/import_declarator.h"
#include "ast/initialize_field.h"
#include "ast/initialize_list.h"
#include "ast/interface_declarator.h"
#include "ast/literal_identifier.h"
#include "ast/literal_string.h"
#include "ast/node_type.h"
#include "ast/program.h"
#include "ast/statement_declaration.h"
#include "ast/statement_expression.h"
#include "ast/statement_import.h"
#include "ast/variable_declarator.h"
#include "astwriter/decorator.h"
#include "astwriter/enum_declarator.h"
#include "astwriter/enum_field.h"
#include "astwriter/expression_assigment.h"
#include "astwriter/expression_binary.h"
#include "astwriter/expression_call.h"
#include "astwriter/expression_comma.h"
#include "astwriter/expression_compute_member.h"
#include "astwriter/expression_group.h"
#include "astwriter/expression_member.h"
#include "astwriter/expression_slice.h"
#include "astwriter/expression_spread.h"
#include "astwriter/expression_template_generator.h"
#include "astwriter/function_argument.h"
#include "astwriter/function_body.h"
#include "astwriter/function_declarator.h"
#include "astwriter/import_declarator.h"
#include "astwriter/initialize_field.h"
#include "astwriter/initialize_list.h"
#include "astwriter/interface_declarator.h"
#include "astwriter/literal_char.h"
#include "astwriter/literal_identifier.h"
#include "astwriter/literal_numeric.h"
#include "astwriter/literal_string.h"
#include "astwriter/literal_symbol.h"
#include "astwriter/program.h"
#include "astwriter/statement_declaration.h"
#include "astwriter/statement_expression.h"
#include "astwriter/statement_import.h"
#include "astwriter/variable_declarator.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/value.h"

cubec_value_t cubec_write_ast_node(cubec_ast_node_t self,
                                   cubec_allocator_t allocator) {
  cubec_value_t value = NULL;
  switch (self->type) {
  case CUBEC_NODE_TYPE_ERROR:
    return NULL;
  case CUBEC_NODE_TYPE_LITERAL_IDENTIFIER:
    value = cubec_write_ast_literal_identifier(
        allocator, (cubec_ast_literal_identifier_t)self);
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_LITERAL_IDENTIFIER"));
    break;
  case CUBEC_NODE_TYPE_LITERAL_NUMERIC:
    value = cubec_write_ast_literal_numeric(allocator,
                                            (cubec_ast_literal_numeric_t)self);
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_LITERAL_NUMERIC"));
    break;
  case CUBEC_NODE_TYPE_LITERAL_STRING:
    value = cubec_write_ast_literal_string(allocator,
                                           (cubec_ast_literal_string_t)self);
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_LITERAL_STRING"));
    break;
  case CUBEC_NODE_TYPE_LITERAL_SYMBOL:
    value = cubec_write_ast_literal_symbol(allocator,
                                           (cubec_ast_literal_symbol_t)self);
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_LITERAL_SYMBOL"));
    break;
  case CUBEC_NODE_TYPE_LITERAL_CHAR:
    value =
        cubec_write_ast_literal_char(allocator, (cubec_ast_literal_char_t)self);
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_LITERAL_CHAR"));
    break;
  case CUBEC_NODE_TYPE_STATEMENT_TYPE:
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_STATEMENT_TYPE"));
    break;
  case CUBEC_NODE_TYPE_STATEMENT_EMPTY:
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_STATEMENT_EMPTY"));
    break;
  case CUBEC_NODE_TYPE_STATEMENT_BLOCK:
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_STATEMENT_BLOCK"));
    break;
  case CUBEC_NODE_TYPE_STATEMENT_IMPORT:
    value = cubec_write_ast_statement_import(
        allocator, (cubec_ast_statement_import_t)self);
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_STATEMENT_IMPORT"));
    break;
  case CUBEC_NODE_TYPE_IMPORT_DECLARATOR:
    value = cubec_write_ast_import_declarator(
        allocator, (cubec_ast_import_declarator)self);
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_IMPORT_DECLARATOR"));
    break;
  case CUBEC_NODE_TYPE_IMPORT_NAMESPACE:
    value = cubec_write_ast_import_declarator(
        allocator, (cubec_ast_import_declarator)self);
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_IMPORT_NAMESPACE"));
    break;
  case CUBEC_NODE_TYPE_STATEMENT_EXPORT:
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_STATEMENT_EXPORT"));
    break;
  case CUBEC_NODE_TYPE_STATEMENT_FUNCTION:
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_STATEMENT_FUNCTION"));
    break;
  case CUBEC_NODE_TYPE_STATEMENT_STRUCT:
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_STATEMENT_STRUCT"));
    break;
  case CUBEC_NODE_TYPE_STATEMENT_ENUM:
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_STATEMENT_ENUM"));
    break;
  case CUBEC_NODE_TYPE_ENUM_FIELD:
    value = cubec_write_ast_enum_field(allocator, (cubec_ast_enum_field_t)self);
    cubec_value_set_field(value, allocator, "type",
                          cubec_value_set_string(cubec_create_value(allocator),
                                                 allocator,
                                                 "CUBEC_NODE_TYPE_ENUM_FIELD"));
    break;
  case CUBEC_NODE_TYPE_STATEMENT_DECLARATION:
    value = cubec_write_ast_statement_declaration(
        allocator, (cubec_ast_statement_declaration_t)self);
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_STATEMENT_DECLARATION"));
    break;
  case CUBEC_NODE_TYPE_VARIABLE_DECLARATOR:
    value = cubec_write_ast_variable_declarator(
        allocator, (cubec_ast_variable_declarator_t)self);
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_VARIABLE_DECLARATOR"));
    break;
  case CUBEC_NODE_TYPE_STATEMENT_EXPRESSION:
    value = cubec_write_ast_statement_expression(
        allocator, (cubec_ast_statement_expression_t)self);
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_STATEMENT_EXPRESSION"));
    break;
  case CUBEC_NODE_TYPE_STATEMENT_IF:
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_STATEMENT_IF"));
    break;
  case CUBEC_NODE_TYPE_IF_ELIF:
    cubec_value_set_field(value, allocator, "type",
                          cubec_value_set_string(cubec_create_value(allocator),
                                                 allocator,
                                                 "CUBEC_NODE_TYPE_IF_ELIF"));
    break;
  case CUBEC_NODE_TYPE_IF_ELSE:
    cubec_value_set_field(value, allocator, "type",
                          cubec_value_set_string(cubec_create_value(allocator),
                                                 allocator,
                                                 "CUBEC_NODE_TYPE_IF_ELSE"));
    break;
  case CUBEC_NODE_TYPE_STATEMENT_SWITCH:
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_STATEMENT_SWITCH"));
    break;
  case CUBEC_NODE_TYPE_SWITCH_CASE:
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_SWITCH_CASE"));
    break;
  case CUBEC_NODE_TYPE_STATEMENT_WHILE:
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_STATEMENT_WHILE"));
    break;
  case CUBEC_NODE_TYPE_STATEMENT_DO_WHILE:
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_STATEMENT_DO_WHILE"));
    break;
  case CUBEC_NODE_TYPE_STATEMENT_FOR:
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_STATEMENT_FOR"));
    break;
  case CUBEC_NODE_TYPE_STATEMENT_FOREACH:
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_STATEMENT_FOREACH"));
    break;
  case CUBEC_NODE_TYPE_STATEMENT_DEFER:
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_STATEMENT_DEFER"));
    break;
  case CUBEC_NODE_TYPE_STATEMENT_BREAK:
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_STATEMENT_BREAK"));
    break;
  case CUBEC_NODE_TYPE_STATEMENT_CONTINUE:
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_STATEMENT_CONTINUE"));
    break;
  case CUBEC_NODE_TYPE_STATEMENT_LABELED:
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_STATEMENT_LABELED"));
    break;
  case CUBEC_NODE_TYPE_STATEMENT_MATCH:
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_STATEMENT_MATCH"));
    break;
  case CUBEC_NODE_TYPE_STATEMENT_TEST:
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_STATEMENT_TEST"));
    break;
  case CUBEC_NODE_TYPE_EXPRESSION_ASSIGMENT:
    value = cubec_write_ast_expression_assigment(
        allocator, (cubec_ast_expression_assigment_t)self);
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_EXPRESSION_ASSIGMENT"));
    break;
  case CUBEC_NODE_TYPE_EXPRESSION_BINARY:
    value = cubec_write_ast_expression_binary(
        allocator, (cubec_ast_expression_binary_t)self);
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_EXPRESSION_BINARY"));
    break;
  case CUBEC_NODE_TYPE_EXPRESSION_CALL:
    value = cubec_write_ast_expression_call(allocator,
                                            (cubec_ast_expression_call_t)self);
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_EXPRESSION_CALL"));
    break;
  case CUBEC_NODE_TYPE_EXPRESSION_MEMBER:
    value = cubec_write_ast_expression_member(
        allocator, (cubec_ast_expression_member_t)self);
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_EXPRESSION_MEMBER"));
    break;
  case CUBEC_NODE_TYPE_EXPRESSION_COMPUTE_MEMBER:
    value = cubec_write_ast_expression_compute_member(
        allocator, (cubec_ast_expression_compute_member_t)self);
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_EXPRESSION_COMPUTE_MEMBER"));
    break;
  case CUBEC_NODE_TYPE_EXPRESSION_TEMPLATE_GENERATOR:
    value = cubec_write_ast_expression_template_generator(
        allocator, (cubec_ast_expression_template_generator_t)self);
    cubec_value_set_field(value, allocator, "type",
                          cubec_value_set_string(
                              cubec_create_value(allocator), allocator,
                              "CUBEC_NODE_TYPE_EXPRESSION_TEMPLATE_GENERATOR"));
    break;
  case CUBEC_NODE_TYPE_EXPRESSION_CONDITION:
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_EXPRESSION_CONDITION"));
    break;
  case CUBEC_NODE_TYPE_EXPRESSION_DELETE:
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_EXPRESSION_DELETE"));
    break;
  case CUBEC_NODE_TYPE_EXPRESSION_GROUP:
    value = cubec_write_ast_expression_group(
        allocator, (cubec_ast_expression_group_t)self);
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_EXPRESSION_GROUP"));
    break;
  case CUBEC_NODE_TYPE_EXPRESSION_SPREAD:
    value = cubec_write_ast_expression_spread(
        allocator, (cubec_ast_expression_spread_t)self);
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_EXPRESSION_SPREAD"));
    break;
  case CUBEC_NODE_TYPE_PROGRAM:
    value = cubec_write_ast_program(allocator, (cubec_ast_program_t)self);
    cubec_value_set_field(value, allocator, "type",
                          cubec_value_set_string(cubec_create_value(allocator),
                                                 allocator,
                                                 "CUBEC_NODE_TYPE_PROGRAM"));
    break;
  case CUBEC_NODE_TYPE_EXPRESSION_COMMON:
    value = cubec_write_ast_expression_comma(
        allocator, (cubec_ast_expression_comma_t)self);
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_EXPRESSION_COMMON"));
    break;
  case CUBEC_NODE_TYPE_STRUCT_DECLARATOR:
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_STRUCT_DECLARATOR"));
    break;
  case CUBEC_NODE_TYPE_ENUM_DECLARATOR:
    value = cubec_write_ast_enum_declarator(allocator,
                                            (cubec_ast_enum_declarator_t)self);
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_ENUM_DECLARATOR"));
    break;
  case CUBEC_NODE_TYPE_FUNCTION_DECLARATOR:
    value = cubec_write_ast_function_declarator(
        allocator, (cubec_ast_function_declarator_t)self);
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_FUNCTION_DECLARATOR"));
    break;
  case CUBEC_NODE_TYPE_FUNCTION_ARGUMENT:
    value = cubec_write_ast_function_argument(
        allocator, (cubec_ast_function_argument_t)self);
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_FUNCTION_ARGUMENT"));
    break;
  case CUBEC_NODE_TYPE_FUNCTION_BODY:
    value = cubec_write_ast_function_body(allocator,
                                          (cubec_ast_function_body_t)self);
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_FUNCTION_BODY"));
    break;
  case CUBEC_NODE_TYPE_INITIALIZE_LIST:
    value = cubec_write_ast_initialize_list(allocator,
                                            (cubec_ast_initialize_list_t)self);
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_INITIALIZE_LIST"));
    break;
  case CUBEC_NODE_TYPE_INITIALIZE_FIELD:
    value = cubec_write_ast_initialize_field(
        allocator, (cubec_ast_initialize_field_t)self);
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_INITIALIZE_FIELD"));
    break;
  case CUBEC_NODE_TYPE_DECORATOR:
    value = cubec_write_ast_decorator(allocator, (cubec_ast_decorator_t)self);
    cubec_value_set_field(value, allocator, "type",
                          cubec_value_set_string(cubec_create_value(allocator),
                                                 allocator,
                                                 "CUBEC_NODE_TYPE_DECORATOR"));
    break;
  case CUBEC_NODE_TYPE_INTERFACE_DECLARATOR:
    value = cubec_write_ast_interface_declarator(
        allocator, (cubec_ast_interface_declarator_t)self);
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_INTERFACE_DECLARATOR"));
    break;
  case CUBEC_NODE_TYPE_EXPRESSION_SLICE:
    value = cubec_write_ast_expression_slice(
        allocator, (cubec_ast_expression_slice_t)self);
    cubec_value_set_field(
        value, allocator, "type",
        cubec_value_set_string(cubec_create_value(allocator), allocator,
                               "CUBEC_NODE_TYPE_EXPRESSION_SLICE"));
    break;
  }
  // cubec_value_t location =
  //     cubec_value_set_object(cubec_create_value(allocator), allocator);
  // cubec_value_t begin =
  //     cubec_value_set_object(cubec_create_value(allocator), allocator);
  // cubec_value_t end =
  //     cubec_value_set_object(cubec_create_value(allocator), allocator);
  // cubec_value_set_field(begin, allocator, "line",
  //                       cubec_value_set_number(cubec_create_value(allocator),
  //                                              allocator,
  //                                              self->loc.begin.line));
  // cubec_value_set_field(begin, allocator, "column",
  //                       cubec_value_set_number(cubec_create_value(allocator),
  //                                              allocator,
  //                                              self->loc.begin.column));
  // cubec_value_set_field(end, allocator, "line",
  //                       cubec_value_set_number(cubec_create_value(allocator),
  //                                              allocator,
  //                                              self->loc.end.line));
  // cubec_value_set_field(end, allocator, "column",
  //                       cubec_value_set_number(cubec_create_value(allocator),
  //                                              allocator,
  //                                              self->loc.end.column));
  // cubec_value_set_field(location, allocator, "begin", begin);
  // cubec_value_set_field(location, allocator, "end", end);
  // cubec_value_set_field(value, allocator, "location", location);
  char *src = cubec_location_get(self->loc, allocator);
  cubec_value_t text =
      cubec_value_set_string(cubec_create_value(allocator), allocator, src);
  cubec_allocator_free(allocator, src);
  cubec_value_set_field(value, allocator, "text", text);
  return value;
}