#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/program.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/compare.h"
#include "core/hash.h"
#include "core/hash_map.h"
#include "core/list.h"
#include "core/location.h"
#include "core/position.h"
#include "core/string.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <unicode/uchar.h>
#include <unicode/umachine.h>

static void ast_node_dispose(ast_node_t self, allocator_t allocator) {
  if (self->type == CUBEC_NODE_TYPE_LIST) {
    allocator_free(allocator, self->items);
  } else {
    allocator_free(allocator, self->children);
  }
}
ast_node_t create_ast_node(allocator_t allocator, size_t type) {
  ast_node_t node = allocator_alloc(allocator, sizeof(struct _ast_node_t),
                                    (dispose_fn_t)ast_node_dispose);
  memset(node, 0, sizeof(struct _ast_node_t));
  node->type = type;
  if (type == CUBEC_NODE_TYPE_LIST) {
    array_initialize_t initialize = {
        .autofree = true,
    };
    node->items = create_array(allocator, &initialize);
  } else {
    hash_map_initialize_t initialize = {
        .autofree_key = true,
        .autofree_value = true,
        .hash = (hash_fn_t)cstring_sdb,
        .compare = (compare_fn_t)strcmp,
    };
    node->children = create_hash_map(allocator, &initialize);
  }
  node->changed = false;
  return node;
}
void ast_add_child(allocator_t allocator, ast_node_t node, const char *name,
                   ast_node_t child) {
  hash_map_set(node->children, create_cstring(allocator, name), child, NULL,
               NULL);
  child->parent = node;
  node->changed = true;
}
void ast_remove_child(ast_node_t node, const char *name) {
  hash_map_delete(node->children, name, NULL, NULL);
  node->changed = true;
}
ast_node_t ast_move_child(ast_node_t node, const char *name) {
  node->changed = true;
  return hash_map_move(node->children, name, NULL, NULL);
}
ast_node_t ast_get_child(ast_node_t node, const char *name) {
  return hash_map_get(node->children, name, NULL, NULL);
}
ast_node_t ast_get_item(ast_node_t node, size_t idx) {
  return array_get(node->items, idx);
}
const char *ast_get_child_name(ast_node_t node, ast_node_t child) {
  list_node_t it = hash_map_get_first(node->children);
  while (it != hash_map_get_end(node->children)) {
    ast_node_t val = hash_map_node_get_value(it);
    if (val == child) {
      return hash_map_node_get_key(it);
    }
    it = hash_map_node_get_next(it);
  }
  return NULL;
}
size_t ast_get_item_index(ast_node_t node, ast_node_t child) {
  size_t idx = 0;
  while (idx < array_get_size(node->items)) {
    if (array_get(node->items, idx) == child) {
      return idx;
    }
    idx++;
  }
  return (size_t)idx;
}
void ast_add_item(ast_node_t node, ast_node_t item) {
  array_push(node->items, item);
  item->parent = node;
  node->changed = true;
}
void ast_remove_item(ast_node_t node, size_t idx) {
  array_del(node->items, idx);
  node->changed = true;
}
ast_node_t ast_move_item(ast_node_t node, size_t idx) {
  node->changed = true;
  ast_node_t current = array_move(node->items, idx);
  current->parent = NULL;
  return current;
}
ast_node_t ast_replace_item(ast_node_t node, size_t idx, ast_node_t item) {
  node->changed = true;
  return array_replace(node->items, idx, item);
}
void ast_insert_item(ast_node_t node, size_t pos, ast_node_t item) {
  node->changed = true;
  array_insert(node->items, pos, item);
  item->parent = node;
}
size_t ast_get_length(ast_node_t node) { return array_get_size(node->items); }
void ast_set_item(ast_node_t node, size_t idx, ast_node_t item) {
  array_set(node->items, idx, item);
  item->parent = node;
  node->changed = true;
}
void ast_set_child(allocator_t allocator, ast_node_t node, const char *name,
                   ast_node_t child) {
  hash_map_set(node->children, create_cstring(allocator, name), child, NULL,
               NULL);
  child->parent = node;
  node->changed = true;
}

int32_t ast_read_code(position_t *position, const char *end,
                      const char *filename) {
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

static void error_dispose(ast_error_t self, allocator_t allocator) {
  allocator_free(allocator, self->message);
  ast_node_dispose(&self->super, allocator);
}

ast_node_t create_ast_error(allocator_t allocator, position_t begin,
                            position_t end, const char *filename,
                            const char *message) {
  ast_error_t node = allocator_alloc(allocator, sizeof(struct _ast_error_t),
                                     (dispose_fn_t)error_dispose);
  memset(node, 0, sizeof(struct _ast_error_t));
  node->super.type = CUBEC_NODE_TYPE_ERROR;
  node->super.loc.begin = begin;
  node->super.loc.end = end;
  node->super.loc.filename = filename;
  size_t len = strlen(message);
  node->message = allocator_alloc(allocator, len + 1, NULL);
  strcpy(node->message, message);
  return &node->super;
}

ast_node_t ast_skip_all(allocator_t allocator, position_t *position,
                        const char *end, const char *filename) {
  position_t current = *position;
  while (*current.offset) {
    int32_t code = ast_read_code(&current, end, filename);
    if (code < 0) {
      return create_ast_error(allocator, *position, current, filename,
                              "invalid unicode code");
    }
    if (u_isWhitespace(code)) {
      *position = current;
      continue;
    }
    if (code == '/') {
      code = ast_read_code(&current, end, filename);
      if (code < 0) {
        return create_ast_error(allocator, *position, current, filename,
                                "invalid unicode code");
      }
      if (code == '/') {
        while (code != '\n' && code != '\r' && code != 0x2028 &&
               code != 0x2029) {
          if (*current.offset == 0) {
            break;
          }
          code = ast_read_code(&current, end, filename);
          if (code < 0) {
            return create_ast_error(allocator, *position, current, filename,
                                    "invalid unicode code");
          }
        }
        *position = current;
        continue;
      }
      if (code == '*') {
        while (true) {
          if (*current.offset == 0) {
            return create_ast_error(allocator, *position, current, filename,
                                    "Missing multiline comment end '*/'");
          }
          code = ast_read_code(&current, end, filename);
          if (code < 0) {
            return create_ast_error(allocator, *position, current, filename,
                                    "invalid unicode code");
          }
          if (code == '\\') {
            if (!*current.offset) {
              return create_ast_error(allocator, *position, current, filename,
                                      "Missing multiline comment end '*/'");
            }
            code = ast_read_code(&current, end, filename);
            if (code < 0) {
              return create_ast_error(allocator, *position, current, filename,
                                      "invalid unicode code");
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

static const char *type_names[] = {
    "CUBEC_NODE_TYPE_LIST",
    "CUBEC_NODE_TYPE_ERROR",
    "CUBEC_NODE_TYPE_LITERAL_IDENTIFIER",
    "CUBEC_NODE_TYPE_LITERAL_NUMERIC",
    "CUBEC_NODE_TYPE_LITERAL_NUMERIC_VALUE",
    "CUBEC_NODE_TYPE_LITERAL_STRING",
    "CUBEC_NODE_TYPE_LITERAL_SYMBOL",
    "CUBEC_NODE_TYPE_LITERAL_CHAR",
    "CUBEC_NODE_TYPE_STATEMENT_EMPTY",
    "CUBEC_NODE_TYPE_STATEMENT_BLOCK",
    "CUBEC_NODE_TYPE_STATEMENT_IMPORT",
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

static char *encode_text(allocator_t allocator, const char *source) {
  char *res = allocator_alloc(allocator, strlen(source) * 2 + 1, NULL);
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

static void print_node(ast_node_t node, allocator_t allocator, string_t out) {
  if (node->type == CUBEC_NODE_TYPE_LIST) {

    string_concat(out, allocator, "[");
    for (size_t idx = 0; idx < array_get_size(node->items); idx++) {
      if (idx != 0) {
        string_concat(out, allocator, ",");
      }
      ast_node_t item = array_get(node->items, idx);
      print_node(item, allocator, out);
    }
    string_concat(out, allocator, "]");
  } else {
    string_concat(out, allocator, "{");
    if (node->type < CUBEC_NODE_TYPE_MAX) {
      char s[strlen(type_names[node->type]) + 32];
      sprintf(s, "\"node_type\":\"%s\"", type_names[node->type]);
      string_concat(out, allocator, s);
    } else {
      char s[128];
      sprintf(s, "\"node_type\":\"CUBEC_NODE_TYPE_MAX + %" PRIuPTR "\"",
              (size_t)node->type - CUBEC_NODE_TYPE_MAX);
      string_concat(out, allocator, s);
    }
    char *text = location_get(node->loc, allocator);
    char *encoded_text = encode_text(allocator, text);
    allocator_free(allocator, text);
    char s[strlen(encoded_text) + 32];
    sprintf(s, ",\"text\":\"%s\"", encoded_text);
    string_concat(out, allocator, s);
    allocator_free(allocator, encoded_text);
    list_node_t it = hash_map_get_first(node->children);
    while (it != hash_map_get_end(node->children)) {
      const char *key = hash_map_node_get_key(it);
      ast_node_t value = hash_map_node_get_value(it);
      if (value) {
        string_concat(out, allocator, ",");
        char s[strlen(key) + 32];
        sprintf(s, "\"%s\":", key);
        string_concat(out, allocator, s);
        print_node(value, allocator, out);
      }
      it = hash_map_node_get_next(it);
    }
    string_concat(out, allocator, "}");
  }
}

char *ast_write_json(allocator_t allocator, ast_node_t node) {
  string_t str = create_string(allocator, NULL);
  print_node(node, allocator, str);
  char *s = create_cstring(allocator, string_get(str));
  allocator_free(allocator, str);
  return s;
}

ast_node_t read_ast_node(allocator_t allocator, const char *filename,
                         const char *source, void *ctx) {
  position_t pos = {
      .column = 1,
      .line = 1,
      .offset = source,
  };
  const char *end = strlen(source) + source;
  ast_node_t program = read_ast_program(allocator, &pos, end, filename);
  if (program->type == CUBEC_NODE_TYPE_ERROR) {
    return program;
  }
  return program;
}
ast_node_t clone_ast_node(allocator_t allocator, ast_node_t node) {
  ast_node_t n = create_ast_node(allocator, node->type);
  n->loc = node->loc;
  if (node->type == CUBEC_NODE_TYPE_LIST) {
    for (size_t idx = 0; idx < ast_get_length(node); idx++) {
      ast_node_t item = ast_get_item(node, idx);
      item = clone_ast_node(allocator, item);
      ast_add_item(n, item);
    }
  } else {
    for (list_node_t it = hash_map_get_first(node->children);
         it != hash_map_get_end(node->children);
         it = hash_map_node_get_next(it)) {
      const char *key = hash_map_node_get_key(it);
      ast_node_t child = hash_map_node_get_value(it);
      child = clone_ast_node(allocator, child);
      ast_add_child(allocator, n, key, child);
    }
  }
  return n;
}