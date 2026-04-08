#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/program.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/compare.h"
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

static void cubec_ast_node_dispose(cubec_ast_node_t self,
                                   cubec_allocator_t allocator) {
  if (self->type == CUBEC_NODE_TYPE_LIST) {
    cubec_allocator_free(allocator, self->items);
  } else {
    cubec_allocator_free(allocator, self->children);
  }
}
cubec_ast_node_t cubec_create_ast_node(cubec_allocator_t allocator,
                                       size_t type) {
  cubec_ast_node_t node =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_ast_node_t),
                            (cubec_dispose_fn_t)cubec_ast_node_dispose);
  memset(node, 0, sizeof(struct _cubec_ast_node_t));
  node->type = type;
  if (type == CUBEC_NODE_TYPE_LIST) {
    cubec_array_initialize_t initialize = {
        .autofree = true,
    };
    node->items = cubec_create_array(allocator, &initialize);
  } else {
    cubec_map_initialize_t initialize = {
        .autofree_key = true,
        .autofree_value = true,
        .compare = (cubec_compare_fn_t)strcmp,
    };
    node->children = cubec_create_map(allocator, &initialize);
  }
  node->changed = false;
  return node;
}
void cubec_ast_add_child(cubec_allocator_t allocator, cubec_ast_node_t node,
                         const char *name, cubec_ast_node_t child) {
  cubec_map_set(node->children, cubec_create_cstring(allocator, name), child,
                NULL);
  child->parent = node;
  node->changed = true;
}
void cubec_ast_remove_child(cubec_ast_node_t node, const char *name) {
  cubec_map_delete(node->children, name, NULL);
  node->changed = true;
}
cubec_ast_node_t cubec_ast_move_child(cubec_ast_node_t node, const char *name) {
  node->changed = true;
  return cubec_map_move(node->children, name, NULL);
}
cubec_ast_node_t cubec_ast_get_child(cubec_ast_node_t node, const char *name) {
  return cubec_map_get(node->children, name, NULL);
}
cubec_ast_node_t cubec_ast_get_item(cubec_ast_node_t node, size_t idx) {
  return cubec_array_get(node->items, idx);
}
const char *cubec_ast_get_child_name(cubec_ast_node_t node,
                                     cubec_ast_node_t child) {
  cubec_list_node_t it = cubec_map_get_first(node->children);
  while (it != cubec_map_get_end(node->children)) {
    cubec_ast_node_t val = cubec_map_node_get_value(it);
    if (val == child) {
      return cubec_map_node_get_key(it);
    }
    it = cubec_map_node_get_next(it);
  }
  return NULL;
}
size_t cubec_ast_get_item_index(cubec_ast_node_t node, cubec_ast_node_t child) {
  size_t idx = 0;
  while (idx < cubec_array_get_size(node->items)) {
    if (cubec_array_get(node->items, idx) == child) {
      return idx;
    }
    idx++;
  }
  return (size_t)idx;
}
void cubec_ast_add_item(cubec_ast_node_t node, cubec_ast_node_t item) {
  cubec_array_push(node->items, item);
  item->parent = node;
  node->changed = true;
}
void cubec_ast_remove_item(cubec_ast_node_t node, size_t idx) {
  cubec_array_del(node->items, idx);
  node->changed = true;
}
cubec_ast_node_t cubec_ast_move_item(cubec_ast_node_t node, size_t idx) {
  node->changed = true;
  cubec_ast_node_t current = cubec_array_move(node->items, idx);
  current->parent = NULL;
  return current;
}
cubec_ast_node_t cubec_ast_replace_item(cubec_ast_node_t node, size_t idx,
                                        cubec_ast_node_t item) {
  node->changed = true;
  return cubec_array_replace(node->items, idx, item);
}
void cubec_ast_insert_item(cubec_ast_node_t node, size_t pos,
                           cubec_ast_node_t item) {
  node->changed = true;
  cubec_array_insert(node->items, pos, item);
  item->parent = node;
}
size_t cubec_ast_get_length(cubec_ast_node_t node) {
  return cubec_array_get_size(node->items);
}
void cubec_ast_set_item(cubec_ast_node_t node, size_t idx,
                        cubec_ast_node_t item) {
  cubec_array_set(node->items, idx, item);
  item->parent = node;
  node->changed = true;
}
void cubec_ast_set_child(cubec_allocator_t allocator, cubec_ast_node_t node,
                         const char *name, cubec_ast_node_t child) {
  cubec_map_set(node->children, cubec_create_cstring(allocator, name), child,
                NULL);
  child->parent = node;
  node->changed = true;
}

int32_t cubec_ast_read_code(cubec_position_t *position, const char *end,
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

static void cubec_error_dispose(cubec_ast_error_t self,
                                cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->message);
  cubec_ast_node_dispose(&self->super, allocator);
}

cubec_ast_node_t cubec_create_ast_error(cubec_allocator_t allocator,
                                        cubec_position_t begin,
                                        cubec_position_t end,
                                        const char *filename,
                                        const char *message) {
  cubec_ast_error_t node =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_ast_error_t),
                            (cubec_dispose_fn_t)cubec_error_dispose);
  memset(node, 0, sizeof(struct _cubec_ast_error_t));
  node->super.type = CUBEC_NODE_TYPE_ERROR;
  node->super.loc.begin = begin;
  node->super.loc.end = end;
  node->super.loc.filename = filename;
  size_t len = strlen(message);
  node->message = cubec_allocator_alloc(allocator, len + 1, NULL);
  strcpy(node->message, message);
  return &node->super;
}

cubec_ast_node_t cubec_ast_skip_all(cubec_allocator_t allocator,
                                    cubec_position_t *position, const char *end,
                                    const char *filename) {
  cubec_position_t current = *position;
  while (*current.offset) {
    int32_t code = cubec_ast_read_code(&current, end, filename);
    if (code < 0) {
      return cubec_create_ast_error(allocator, *position, current, filename,
                                    "Invalid unicode code");
    }
    if (u_isWhitespace(code)) {
      *position = current;
      continue;
    }
    if (code == '/') {
      code = cubec_ast_read_code(&current, end, filename);
      if (code < 0) {
        return cubec_create_ast_error(allocator, *position, current, filename,
                                      "Invalid unicode code");
      }
      if (code == '/') {
        while (code != '\n' && code != '\r' && code != 0x2028 &&
               code != 0x2029) {
          if (*current.offset == 0) {
            break;
          }
          code = cubec_ast_read_code(&current, end, filename);
          if (code < 0) {
            return cubec_create_ast_error(allocator, *position, current,
                                          filename, "Invalid unicode code");
          }
        }
        *position = current;
        continue;
      }
      if (code == '*') {
        while (true) {
          if (*current.offset == 0) {
            return cubec_create_ast_error(allocator, *position, current,
                                          filename,
                                          "Missing multiline comment end '*/'");
          }
          code = cubec_ast_read_code(&current, end, filename);
          if (code < 0) {
            return cubec_create_ast_error(allocator, *position, current,
                                          filename, "Invalid unicode code");
          }
          if (code == '\\') {
            if (!*current.offset) {
              return cubec_create_ast_error(
                  allocator, *position, current, filename,
                  "Missing multiline comment end '*/'");
            }
            code = cubec_ast_read_code(&current, end, filename);
            if (code < 0) {
              return cubec_create_ast_error(allocator, *position, current,
                                            filename, "Invalid unicode code");
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

    cubec_string_concat(out, allocator, "[");
    for (size_t idx = 0; idx < cubec_array_get_size(node->items); idx++) {
      if (idx != 0) {
        cubec_string_concat(out, allocator, ",");
      }
      cubec_ast_node_t item = cubec_array_get(node->items, idx);
      cubec_print_node(item, allocator, out);
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
              (size_t)node->type - CUBEC_NODE_TYPE_MAX);
      cubec_string_concat(out, allocator, s);
    }
    char *text = cubec_location_get(node->loc, allocator);
    char *encoded_text = encode_text(allocator, text);
    cubec_allocator_free(allocator, text);
    char s[strlen(encoded_text) + 32];
    sprintf(s, ",\"text\":\"%s\"", encoded_text);
    cubec_string_concat(out, allocator, s);
    cubec_allocator_free(allocator, encoded_text);
    cubec_list_node_t it = cubec_map_get_first(node->children);
    while (it != cubec_map_get_end(node->children)) {
      const char *key = cubec_map_node_get_key(it);
      cubec_ast_node_t value = cubec_map_node_get_value(it);
      if (value) {
        cubec_string_concat(out, allocator, ",");
        char s[strlen(key) + 32];
        sprintf(s, "\"%s\":", key);
        cubec_string_concat(out, allocator, s);
        cubec_print_node(value, allocator, out);
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

cubec_ast_node_t cubec_visit_ast_node(cubec_allocator_t allocator,
                                      cubec_ast_node_t node, void *ctx,
                                      cubec_visit_ast_fn_t visit) {
  node->changed = false;
  node = visit(allocator, node, ctx);
  if (node->changed) {
    return node;
  }
  if (node->type == CUBEC_NODE_TYPE_ERROR) {
    return node;
  }
  if (node->type == CUBEC_NODE_TYPE_LIST) {
    size_t idx = 0;
    while (idx < cubec_array_get_size(node->items)) {
      cubec_ast_node_t item = cubec_array_get(node->items, idx);
      item = cubec_visit_ast_node(allocator, item, ctx, visit);
      if (item == node && item->changed) {
        idx = 0;
        node->changed = false;
        continue;
      }
      if (item->changed) {
        return node;
      }
      if (item->type == CUBEC_NODE_TYPE_ERROR) {
        return item;
      }
      idx++;
    }
  } else {
    cubec_list_node_t it = cubec_map_get_first(node->children);
    while (it != cubec_map_get_end(node->children)) {
      cubec_ast_node_t item = cubec_map_node_get_value(it);
      item = cubec_visit_ast_node(allocator, item, ctx, visit);
      if (item == node && item->changed) {
        it = cubec_map_get_first(node->children);
        item->changed = false;
        continue;
      }
      if (item->changed) {
        return node;
      }
      if (item->type == CUBEC_NODE_TYPE_ERROR) {
        return item;
      }
      it = cubec_list_node_next(it);
    }
  }
  return node;
}

cubec_ast_node_t cubec_read_ast_node(cubec_allocator_t allocator,
                                     const char *filename, const char *source,
                                     void *ctx, cubec_visit_ast_fn_t visit) {
  cubec_position_t pos = {
      .column = 1,
      .line = 1,
      .offset = source,
  };
  const char *end = strlen(source) + source;
  cubec_ast_node_t program =
      cubec_read_ast_program(allocator, &pos, end, filename);
  if (program->type == CUBEC_NODE_TYPE_ERROR) {
    return program;
  }
  return cubec_visit_ast_node(allocator, program, ctx, visit);
}
cubec_ast_node_t cubec_clone_ast_node(cubec_allocator_t allocator,
                                      cubec_ast_node_t node) {
  cubec_ast_node_t n = cubec_create_ast_node(allocator, node->type);
  n->loc = node->loc;
  if (node->type == CUBEC_NODE_TYPE_LIST) {
    for (size_t idx = 0; idx < cubec_ast_get_length(node); idx++) {
      cubec_ast_node_t item = cubec_ast_get_item(node, idx);
      item = cubec_clone_ast_node(allocator, item);
      cubec_ast_add_item(n, item);
    }
  } else {
    for (cubec_list_node_t it = cubec_map_get_first(node->children);
         it != cubec_map_get_end(node->children);
         it = cubec_map_node_get_next(it)) {
      const char *key = cubec_map_node_get_key(it);
      cubec_ast_node_t child = cubec_map_node_get_value(it);
      child = cubec_clone_ast_node(allocator, child);
      cubec_ast_add_child(allocator, n, key, child);
    }
  }
  return n;
}