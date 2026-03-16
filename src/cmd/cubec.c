#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/program.h"
#include "core/allocator.h"
#include "core/list.h"
#include "core/location.h"
#include "core/map.h"
#include "core/path.h"
#include "core/position.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

static const char *type_names[] = {
    "CUBEC_NODE_TYPE_LIST",
    "CUBEC_NODE_TYPE_ERROR",
    "CUBEC_NODE_TYPE_LITERAL_IDENTIFIER",
    "CUBEC_NODE_TYPE_LITERAL_NUMERIC",
    "CUBEC_NODE_TYPE_LITERAL_STRING",
    "CUBEC_NODE_TYPE_LITERAL_SYMBOL",
    "CUBEC_NODE_TYPE_LITERAL_CHAR",
    "CUBEC_NODE_TYPE_STATEMENT_EMPTY",
    "CUBEC_NODE_TYPE_STATEMENT_BLOCK",
    "CUBEC_NODE_TYPE_STATEMENT_IMPORT",
    "CUBEC_NODE_TYPE_IMPORT_DECLARATOR",
    "CUBEC_NODE_TYPE_IMPORT_NAMESPACE",
    "CUBEC_NODE_TYPE_STATEMENT_FUNCTION",
    "CUBEC_NODE_TYPE_STATEMENT_STRUCT",
    "CUBEC_NODE_TYPE_STATEMENT_ENUM",
    "CUBEC_NODE_TYPE_ENUM_FIELD",
    "CUBEC_NODE_TYPE_STATEMENT_DECLARATION",
    "CUBEC_NODE_TYPE_VARIABLE_DECLARATOR",
    "CUBEC_NODE_TYPE_STATEMENT_EXPRESSION",
    "CUBEC_NODE_TYPE_STATEMENT_IF",
    "CUBEC_NODE_TYPE_STATEMENT_SWITCH",
    "CUBEC_NODE_TYPE_SWITCH_CASE",
    "CUBEC_NODE_TYPE_STATEMENT_WHILE",
    "CUBEC_NODE_TYPE_STATEMENT_DO_WHILE",
    "CUBEC_NODE_TYPE_STATEMENT_FOR",
    "CUBEC_NODE_TYPE_STATEMENT_FOREACH",
    "CUBEC_NODE_TYPE_STATEMENT_DEFER",
    "CUBEC_NODE_TYPE_STATEMENT_BREAK",
    "CUBEC_NODE_TYPE_STATEMENT_CONTINUE",
    "CUBEC_NODE_TYPE_STATEMENT_RETURN",
    "CUBEC_NODE_TYPE_STATEMENT_TEST",
    "CUBEC_NODE_TYPE_ARRAY_DECLARATOR",
    "CUBEC_NODE_TYPE_EXPRESSION_ASSIGMENT",
    "CUBEC_NODE_TYPE_EXPRESSION_BINARY",
    "CUBEC_NODE_TYPE_EXPRESSION_CALL",
    "CUBEC_NODE_TYPE_EXPRESSION_MEMBER",
    "CUBEC_NODE_TYPE_EXPRESSION_COMPUTE_MEMBER",
    "CUBEC_NODE_TYPE_EXPRESSION_TEMPLATE_GENERATOR",
    "CUBEC_NODE_TYPE_EXPRESSION_CONDITION",
    "CUBEC_NODE_TYPE_EXPRESSION_COMMON",
    "CUBEC_NODE_TYPE_EXPRESSION_GROUP",
    "CUBEC_NODE_TYPE_EXPRESSION_SPREAD",
    "CUBEC_NODE_TYPE_STRUCT_DECLARATOR",
    "CUBEC_NODE_TYPE_STRUCT_FIELD",
    "CUBEC_NODE_TYPE_ENUM_DECLARATOR",
    "CUBEC_NODE_TYPE_FUNCTION_DECLARATOR",
    "CUBEC_NODE_TYPE_FUNCTION_BODY",
    "CUBEC_NODE_TYPE_FUNCTION_ARGUMENT",
    "CUBEC_NODE_TYPE_FUNCTION_ARGUMENT_REST",
    "CUBEC_NODE_TYPE_EXPRESSION_SLICE",
    "CUBEC_NODE_TYPE_INITIALIZE_LIST",
    "CUBEC_NODE_TYPE_INITIALIZE_FIELD",
    "CUBEC_NODE_TYPE_PROGRAM",
    "CUBEC_NODE_TYPE_DECORATOR",
    "CUBEC_NODE_TYPE_INTERFACE_DECLARATOR",
    "CUBEC_NODE_TYPE_PTR_DECLARATOR",
    "CUBEC_NODE_TYPE_TYPE",
};

char *encode_text(cubec_allocator_t allocator, const char *source) {
  char *res = cubec_allocator_alloc(allocator, strlen(source) * 2 + 1, NULL);
  const char *src = source;
  char *dst = res;
  while (*src) {
    if (*src == '\"') {
      *dst++ = '\\';
      *dst++ = '\"';
    } else if (*src == '\n') {
      *dst++ = '\\';
      *dst++ = 'n';
    } else if (*src == '\r') {
      *dst++ = '\\';
      *dst++ = 'r';
    } else if (*src == '\t') {
      *dst++ = '\\';
      *dst++ = 't';
    } else if (*src == '\\') {
      *dst++ = '\\';
      *dst++ = '\\';
    } else if (*src == '\f') {
      *dst++ = '\\';
      *dst++ = 'f';
    } else if (*src == '\a') {
      *dst++ = '\\';
      *dst++ = 'a';
    } else if (*src == '\b') {
      *dst++ = '\\';
      *dst++ = 'b';
    } else {
      *dst++ = *src;
    }
    src++;
  }
  *dst = 0;
  return res;
}

char *absolute(cubec_allocator_t allocator, const char *filename) {
  cubec_path_t path = cubec_create_path(allocator, filename);
  path = cubec_path_absolute(path, allocator);
  char *fullname = cubec_path_to_string(path, allocator);
  cubec_allocator_free(allocator, path);
  return fullname;
}

char *read(cubec_allocator_t allocator, const char *fullname) {
  FILE *fp = fopen(fullname, "rb");
  fseek(fp, 0, SEEK_END);
  size_t len = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  char *source = cubec_allocator_alloc(allocator, len + 1, NULL);
  fread(source, len, 1, fp);
  source[len] = 0;
  fclose(fp);
  return source;
}

cubec_ast_node_t compile(cubec_allocator_t allocator, const char *filename,
                         const char *source) {
  cubec_position_t position = {
      .column = 0,
      .line = 0,
      .offset = source,
  };

  return cubec_read_ast_program(allocator, &position, source + strlen(source));
}

cubec_ast_node_t cubec_print_node(cubec_ast_node_t node,
                                  cubec_allocator_t allocator, FILE *out) {
  if (node->type == CUBEC_NODE_TYPE_LIST) {
    cubec_ast_list_node_t list = (cubec_ast_list_node_t)node;
    fprintf(out, "[");
    cubec_list_node_t it = cubec_list_get_first(list->items);
    while (it != cubec_list_get_end(list->items)) {
      if (it != cubec_list_get_first(list->items)) {
        fprintf(out, ",");
      }
      cubec_ast_node_t item = cubec_list_node_get(it);
      cubec_ast_node_t err = cubec_print_node(item, allocator, out);
      if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
        return err;
      }
      it = cubec_list_node_next(it);
    }
    fprintf(out, "]");
  } else {
    fprintf(out, "{");
    if (node->type < CUBEC_NODE_TYPE_MAX) {
      fprintf(out, "\"node_type\":\"%s\"", type_names[node->type]);
    } else {
      fprintf(out, "\"node_type\":\"CUBEC_NODE_TYPE_MAX + %" PRIuPTR "\"",
              node->type - CUBEC_NODE_TYPE_MAX);
    }
    char *text = cubec_location_get(node->loc, allocator);
    char *encoded_text = encode_text(allocator, text);
    cubec_allocator_free(allocator, text);
    fprintf(out, ",\"text\":\"%s\"", encoded_text);
    cubec_allocator_free(allocator, encoded_text);
    cubec_list_node_t it = cubec_map_get_first(node->meta);
    while (it != cubec_map_get_end(node->meta)) {
      const char *key = cubec_map_node_get_key(it);
      cubec_ast_node_t *value = cubec_map_node_get_value(it);
      if (*value) {
        fprintf(out, ",");
        fprintf(out, "\"%s\":", key);
        cubec_ast_node_t err = cubec_print_node(*value, allocator, out);
        if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
          return err;
        }
      }
      it = cubec_map_node_get_next(it);
    }
    fprintf(out, "}");
  }
  return NULL;
}

int main(int argc, char *argv[]) {
  cubec_allocator_t allocator = cubec_create_allocator(NULL);
  char *filename = absolute(allocator, "./main.cubec");
  char *source = read(allocator, filename);
  cubec_ast_node_t node = compile(allocator, filename, source);
  if (node->type == CUBEC_NODE_TYPE_ERROR) {
    cubec_ast_error_t error = (cubec_ast_error_t)node;
    fprintf(stderr, "Failed to compile: %s at\n  %s: %" PRIuPTR ":%" PRIuPTR,
            error->message, "./main.cubec", node->loc.end.line,
            node->loc.end.column);
  } else {
    cubec_ast_node_t err = cubec_visit_node(
        node, allocator, (cubec_ast_visit_fn_t)cubec_print_node, stdout);
    printf("\n");
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      cubec_ast_error_t error = (cubec_ast_error_t)node;
      fprintf(stderr, "Failed to print: %s at\n  %s: %" PRIuPTR ":%" PRIuPTR,
              error->message, "./main.cubec", node->loc.end.line,
              node->loc.end.column);
    }
  }
  cubec_allocator_free(allocator, node);
  cubec_allocator_free(allocator, source);
  cubec_allocator_free(allocator, filename);
  cubec_delete_allocator(allocator);
  return 0;
}