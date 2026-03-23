#include "c/writer.h"
#include "c/program.h"
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
static cubec_writer_fn_t writers[] = {
    NULL, // CUBEC_NODE_TYPE_LIST
    NULL, // CUBEC_NODE_TYPE_ERROR
    NULL, // CUBEC_NODE_TYPE_LITERAL_IDENTIFIER
    NULL, // CUBEC_NODE_TYPE_LITERAL_NUMERIC
    NULL, // CUBEC_NODE_TYPE_LITERAL_STRING
    NULL, // CUBEC_NODE_TYPE_LITERAL_SYMBOL
    NULL, // CUBEC_NODE_TYPE_LITERAL_CHAR
    NULL, // CUBEC_NODE_TYPE_STATEMENT_EMPTY
    NULL, // CUBEC_NODE_TYPE_STATEMENT_BLOCK
    NULL, // CUBEC_NODE_TYPE_STATEMENT_IMPORT
    NULL, // CUBEC_NODE_TYPE_STATEMENT_FUNCTION
    NULL, // CUBEC_NODE_TYPE_STATEMENT_STRUCT
    NULL, // CUBEC_NODE_TYPE_STATEMENT_ENUM
    NULL, // CUBEC_NODE_TYPE_ENUM_FIELD
    NULL, // CUBEC_NODE_TYPE_STATEMENT_DECLARATION
    NULL, // CUBEC_NODE_TYPE_VARIABLE_DECLARATOR
    NULL, // CUBEC_NODE_TYPE_STATEMENT_EXPRESSION
    NULL, // CUBEC_NODE_TYPE_STATEMENT_IF
    NULL, // CUBEC_NODE_TYPE_STATEMENT_SWITCH
    NULL, // CUBEC_NODE_TYPE_SWITCH_CASE
    NULL, // CUBEC_NODE_TYPE_STATEMENT_WHILE
    NULL, // CUBEC_NODE_TYPE_STATEMENT_DO_WHILE
    NULL, // CUBEC_NODE_TYPE_STATEMENT_FOR
    NULL, // CUBEC_NODE_TYPE_STATEMENT_FOREACH
    NULL, // CUBEC_NODE_TYPE_STATEMENT_DEFER
    NULL, // CUBEC_NODE_TYPE_STATEMENT_BREAK
    NULL, // CUBEC_NODE_TYPE_STATEMENT_CONTINUE
    NULL, // CUBEC_NODE_TYPE_STATEMENT_RETURN
    NULL, // CUBEC_NODE_TYPE_STATEMENT_TEST
    NULL, // CUBEC_NODE_TYPE_ARRAY_DECLARATOR
    NULL, // CUBEC_NODE_TYPE_EXPRESSION_ASSIGMENT
    NULL, // CUBEC_NODE_TYPE_EXPRESSION_BINARY
    NULL, // CUBEC_NODE_TYPE_EXPRESSION_CALL
    NULL, // CUBEC_NODE_TYPE_EXPRESSION_MEMBER
    NULL, // CUBEC_NODE_TYPE_EXPRESSION_COMPUTE_MEMBER
    NULL, // CUBEC_NODE_TYPE_EXPRESSION_TEMPLATE_GENERATOR
    NULL, // CUBEC_NODE_TYPE_EXPRESSION_CONDITION
    NULL, // CUBEC_NODE_TYPE_EXPRESSION_COMMON
    NULL, // CUBEC_NODE_TYPE_EXPRESSION_GROUP
    NULL, // CUBEC_NODE_TYPE_EXPRESSION_SPREAD
    NULL, // CUBEC_NODE_TYPE_STRUCT_DECLARATOR
    NULL, // CUBEC_NODE_TYPE_STRUCT_FIELD
    NULL, // CUBEC_NODE_TYPE_ENUM_DECLARATOR
    NULL, // CUBEC_NODE_TYPE_FUNCTION_DECLARATOR
    NULL, // CUBEC_NODE_TYPE_FUNCTION_BODY
    NULL, // CUBEC_NODE_TYPE_FUNCTION_ARGUMENT
    NULL, // CUBEC_NODE_TYPE_FUNCTION_ARGUMENT_REST
    NULL, // CUBEC_NODE_TYPE_EXPRESSION_SLICE
    NULL, // CUBEC_NODE_TYPE_INITIALIZE_LIST
    NULL, // CUBEC_NODE_TYPE_INITIALIZE_FIELD
    (cubec_writer_fn_t)cubec_c_write_program, // CUBEC_NODE_TYPE_PROGRAM
    NULL,                                     // CUBEC_NODE_TYPE_DECORATOR
    NULL, // CUBEC_NODE_TYPE_INTERFACE_DECLARATOR
    NULL, // CUBEC_NODE_TYPE_PTR_DECLARATOR
    NULL, // CUBEC_NODE_TYPE_TYPE
};
cubec_value_t cubec_c_write(cubec_context_t self, cubec_ast_node_t node,
                            const char *filename, cubec_string_t *output) {
  cubec_writer_fn_t writer = writers[node->type];
  if (!writer) {
    return cubec_c_create_error(self, node, filename, "Not implement writer");
  }
  return writer(self, node, filename, output);
}
cubec_value_t cubec_c_create_error(cubec_context_t self, cubec_ast_node_t node,
                                   const char *filename, const char *fmt, ...) {
  char num[16];
  sprintf(num, "%" PRIuPTR " | ", node->loc.begin.line);
  char *line = cubec_location_get_line(node->loc, self->allocator);
  size_t len = strlen(line);
  size_t column = node->loc.begin.column - 1;
  len += strlen(num);
  char marks[len + 1];
  memset(marks, 0, len + 1);
  for (size_t id = 0; id < len; id++) {
    if (id < column + strlen(num)) {
      marks[id] = ' ';
    } else {
      marks[id] = '^';
    }
  }
  va_list args;
  va_start(args, fmt);
  len = vsnprintf(NULL, 0, fmt, args);
  char *msg = cubec_allocator_alloc(self->allocator, len + 1, NULL);
  va_end(args);
  va_start(args, fmt);
  vsprintf(msg, fmt, args);
  va_end(args);
  cubec_value_t err =
      cubec_context_create_error(self,
                                 "%s:%" PRIuPTR ":%" PRIuPTR ": error: %s\n"
                                 "%s%s\n"
                                 "%s\n",
                                 filename, node->loc.begin.line,
                                 node->loc.begin.column, msg, num, line, marks);
  cubec_allocator_free(self->allocator, line);
  cubec_allocator_free(self->allocator, msg);
  return err;
}