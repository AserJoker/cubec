#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/list.h"
#include "core/location.h"
#include "core/map.h"
#include "core/position.h"
#include "core/string.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <unicode/uchar.h>
#include <unicode/umachine.h>

void cubec_ast_node_initialize(cubec_allocator_t allocator,
                               cubec_ast_node_t self) {
  self->type = 0;
  self->loc.begin.offset = NULL;
  self->loc.begin.column = 0;
  self->loc.begin.line = 0;
  self->loc.end.offset = NULL;
  self->loc.end.column = 0;
  self->loc.end.line = 0;
  cubec_map_initialize_t initialize = {
      .autofree_key = true,
      .autofree_value = false,
      .compare = (cubec_compare_fn_t)strcmp,
  };
  self->meta = cubec_create_map(allocator, &initialize);
  self->parent = NULL;
}

void cubec_ast_node_dispose(cubec_allocator_t allocator,
                            cubec_ast_node_t self) {
  cubec_allocator_free(allocator, self->meta);
}

int32_t cubec_ast_read_code(cubec_position_t *position, const char *end) {
  int32_t code = 0;
  size_t offset = 0;
  size_t len = end - position->offset;
  U8_NEXT(position->offset, offset, len, code);
  position->offset += offset;
  if (code >= 0) {
    if (code == '\\' && *position->offset == 'u') {
      position->offset++;
      position->column++;
      if (*position->offset == '{') {
        position->offset++;
        position->column++;
        code = 0;
        while (*position->offset != '}') {
          if (!*position->offset) {
            return U_INVALID_FORMAT_ERROR;
          }
          if (*position->offset >= '0' && *position->offset <= '9') {
            code *= 16 + (*position->offset - '0');
          } else if (*position->offset >= 'a' && *position->offset <= 'f') {
            code *= 16 + (*position->offset - 'a');
          } else if (*position->offset >= 'A' && *position->offset <= 'F') {
            code *= 16 + (*position->offset - 'F');
          } else {
            return U_INVALID_FORMAT_ERROR;
          }
          position->offset++;
          position->column++;
        }
        position->offset++;
        position->column++;
        return code;
      } else {
        code = 0;
        for (int32_t idx = 0; idx < 4; idx++) {
          if (!*position->offset) {
            return U_INVALID_FORMAT_ERROR;
          }
          if (*position->offset >= '0' && *position->offset <= '9') {
            code *= 16 + (*position->offset - '0');
          } else if (*position->offset >= 'a' && *position->offset <= 'f') {
            code *= 16 + (*position->offset - 'a');
          } else if (*position->offset >= 'A' && *position->offset <= 'F') {
            code *= 16 + (*position->offset - 'F');
          } else {
            return U_INVALID_FORMAT_ERROR;
          }
          position->offset++;
          position->column++;
        }
        return code;
      }
    }
    if (code == '\n' || code == 0x2028 || code == 0x2029) {
      position->line++;
      position->column = 1;
      if (*position->offset == '\r') {
        position->offset++;
      }
    } else if (code == '\r') {
      position->line++;
      position->column = 1;
      if (*position->offset == '\n') {
        position->offset++;
      }
    } else {
      position->column += offset;
    }
  } else {
    position->column += offset;
  }
  return code;
}

void cubec_ast_init_field(cubec_ast_node_t self, cubec_allocator_t allocator,
                          const char *name, cubec_ast_node_t *field) {
  cubec_map_set(self->meta, cubec_create_cstring(allocator, name), field, NULL);
  *field = NULL;
}
void cubec_ast_set_parent(cubec_ast_node_t node, cubec_ast_node_t parent) {
  if (node) {
    node->parent = parent;
  }
}

static void cubec_error_dispose(cubec_ast_error_t self,
                                cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->message);
  cubec_ast_node_dispose(allocator, &self->super);
}

cubec_ast_node_t cubec_create_ast_error(cubec_allocator_t allocator,
                                        cubec_position_t begin,
                                        cubec_position_t end,
                                        const char *message) {
  cubec_ast_error_t node =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_ast_error_t),
                            (cubec_dispose_fn_t)cubec_error_dispose);
  cubec_ast_node_initialize(allocator, &node->super);
  node->super.type = CUBEC_NODE_TYPE_ERROR;
  node->super.loc.begin = begin;
  node->super.loc.end = end;
  size_t len = strlen(message);
  node->message = cubec_allocator_alloc(allocator, len + 1, NULL);
  strcpy(node->message, message);
  return &node->super;
}

cubec_ast_node_t cubec_ast_skip_all(cubec_allocator_t allocator,
                                    cubec_position_t *position,
                                    const char *end) {
  cubec_position_t current = *position;
  while (*current.offset) {
    int32_t code = cubec_ast_read_code(&current, end);
    if (code < 0) {
      return cubec_create_ast_error(allocator, *position, current,
                                    "Invalid unicode code");
    }
    if (u_isWhitespace(code)) {
      *position = current;
      continue;
    }
    if (code == '/') {
      code = cubec_ast_read_code(&current, end);
      if (code < 0) {
        return cubec_create_ast_error(allocator, *position, current,
                                      "Invalid unicode code");
      }
      if (code == '/') {
        while (code != '\n' && code != '\r' && code != 0x2028 &&
               code != 0x2029) {
          if (*current.offset == 0) {
            break;
          }
          code = cubec_ast_read_code(&current, end);
          if (code < 0) {
            return cubec_create_ast_error(allocator, *position, current,
                                          "Invalid unicode code");
          }
        }
        *position = current;
        continue;
      }
      if (code == '*') {
        while (true) {
          if (*current.offset == 0) {
            return cubec_create_ast_error(allocator, *position, current,
                                          "Missing multiline comment end '*/'");
          }
          code = cubec_ast_read_code(&current, end);
          if (code < 0) {
            return cubec_create_ast_error(allocator, *position, current,
                                          "Invalid unicode code");
          }
          if (code == '\\') {
            if (!*current.offset) {
              return cubec_create_ast_error(
                  allocator, *position, current,
                  "Missing multiline comment end '*/'");
            }
            code = cubec_ast_read_code(&current, end);
            if (code < 0) {
              return cubec_create_ast_error(allocator, *position, current,
                                            "Invalid unicode code");
            }
            continue;
          }
          if (code == '*' && *current.offset == '/') {
            current.offset++;
            current.column++;
            break;
          }
        }
        *position = current;
        continue;
      }
      break;
    }
    current.offset--;
    current.column--;
    break;
  }
  return NULL;
}
static void cubec_ast_list_node_dispose(cubec_ast_list_node_t self,
                                        cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->items);
  cubec_ast_node_dispose(allocator, &self->super);
}

cubec_ast_node_t cubec_create_ast_list_node(cubec_allocator_t allocator) {
  cubec_ast_list_node_t node =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_ast_list_node_t),
                            (cubec_dispose_fn_t)cubec_ast_list_node_dispose);
  cubec_ast_node_initialize(allocator, &node->super);
  node->super.type = CUBEC_NODE_TYPE_LIST;
  cubec_list_initialize_t initialize = {
      .autofree = true,
  };
  node->items = cubec_create_list(allocator, &initialize);
  return &node->super;
}
void cubec_ast_list_node_append(cubec_ast_node_t self,
                                cubec_allocator_t allocator,
                                cubec_ast_node_t item) {
  cubec_ast_list_node_t list = (cubec_ast_list_node_t)self;
  cubec_list_append(list->items, item);
  item->parent = &list->super;
}

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

static char *encode_text(cubec_allocator_t allocator, const char *source) {
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

static void cubec_print_node(cubec_ast_node_t node, cubec_allocator_t allocator,
                             cubec_string_t out) {
  if (node->type == CUBEC_NODE_TYPE_LIST) {
    cubec_ast_list_node_t list = (cubec_ast_list_node_t)node;
    cubec_string_concat(out, allocator, "[");
    cubec_list_node_t it = cubec_list_get_first(list->items);
    while (it != cubec_list_get_end(list->items)) {
      if (it != cubec_list_get_first(list->items)) {
        cubec_string_concat(out, allocator, ",");
      }
      cubec_ast_node_t item = cubec_list_node_get(it);
      cubec_print_node(item, allocator, out);
      it = cubec_list_node_next(it);
    }
    cubec_string_concat(out, allocator, "]");
  } else {
    cubec_string_concat(out, allocator, "{");
    if (node->type < CUBEC_NODE_TYPE_MAX) {
      char s[strlen(type_names[node->type]) + 32];
      sprintf(s, "\"node_type\":\"%s\"", type_names[node->type]);
      cubec_string_concat(out, allocator, s);
    } else {
      char s[128];
      sprintf(s, "\"node_type\":\"CUBEC_NODE_TYPE_MAX + %" PRIuPTR "\"",
              node->type - CUBEC_NODE_TYPE_MAX);
      cubec_string_concat(out, allocator, s);
    }
    char *text = cubec_location_get(node->loc, allocator);
    char *encoded_text = encode_text(allocator, text);
    cubec_allocator_free(allocator, text);
    char s[strlen(encoded_text) + 32];
    sprintf(s, ",\"text\":\"%s\"", encoded_text);
    cubec_string_concat(out, allocator, s);
    cubec_allocator_free(allocator, encoded_text);
    cubec_list_node_t it = cubec_map_get_first(node->meta);
    while (it != cubec_map_get_end(node->meta)) {
      const char *key = cubec_map_node_get_key(it);
      cubec_ast_node_t *value = cubec_map_node_get_value(it);
      if (*value) {
        cubec_string_concat(out, allocator, ",");
        char s[strlen(key) + 32];
        sprintf(s, "\"%s\":", key);
        cubec_string_concat(out, allocator, s);
        cubec_print_node(*value, allocator, out);
      }
      it = cubec_map_node_get_next(it);
    }
    cubec_string_concat(out, allocator, "}");
  }
}

char *cubec_ast_write_json(cubec_allocator_t allocator, cubec_ast_node_t node) {
  cubec_string_t str = cubec_create_string(allocator, NULL);
  cubec_print_node(node, allocator, str);
  char *s = cubec_create_cstring(allocator, cubec_string_get(str));
  cubec_allocator_free(allocator, str);
  return s;
}