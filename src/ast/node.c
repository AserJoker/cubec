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
#include "reader/token.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <unicode/uchar.h>
#include <unicode/umachine.h>

static void ast_node_dispose(ast_node_t self, allocator_t allocator) {
  allocator_free(allocator, self->data);
}
ast_node_t create_ast_node(allocator_t allocator, size_t type) {
  ast_node_t node = allocator_alloc(allocator, sizeof(struct _ast_node_t),
                                    (dispose_fn_t)ast_node_dispose);
  memset(node, 0, sizeof(struct _ast_node_t));
  node->type = type;
  if (type == NODE_TYPE_LIST) {
    array_initialize_t initialize = {
        .autofree = true,
    };
    node->items = create_array(allocator, &initialize);
  } else if (type > NODE_TYPE_LIST) {
    hash_map_initialize_t initialize = {
        .autofree_key = true,
        .autofree_value = true,
        .hash = (hash_fn_t)cstring_sdb,
        .compare = (compare_fn_t)strcmp,
    };
    node->children = create_hash_map(allocator, &initialize);
  } else {
    node->data = NULL;
  }
  node->visible = true;
  return node;
}

void ast_add_child(allocator_t allocator, ast_node_t node, const char *name,
                   ast_node_t child) {
  if (hash_map_get(node->children, name, NULL, NULL) != child) {
    hash_map_delete(node->children, name, NULL, NULL);
  }
  hash_map_set(node->children, create_cstring(allocator, name), child, NULL,
               NULL);
  child->parent = node;
}
void ast_remove_child(ast_node_t node, const char *name) {
  hash_map_delete(node->children, name, NULL, NULL);
}
ast_node_t ast_move_child(ast_node_t node, const char *name) {
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
}
void ast_remove_item(ast_node_t node, size_t idx) {
  array_del(node->items, idx);
}
ast_node_t ast_move_item(ast_node_t node, size_t idx) {
  ast_node_t current = array_move(node->items, idx);
  current->parent = NULL;
  return current;
}
ast_node_t ast_replace_item(ast_node_t node, size_t idx, ast_node_t item) {
  return array_replace(node->items, idx, item);
}
void ast_insert_item(ast_node_t node, size_t pos, ast_node_t item) {
  array_insert(node->items, pos, item);
  item->parent = node;
}
size_t ast_get_length(ast_node_t node) { return array_get_size(node->items); }
void ast_set_item(ast_node_t node, size_t idx, ast_node_t item) {
  array_set(node->items, idx, item);
  item->parent = node;
}
void ast_set_child(allocator_t allocator, ast_node_t node, const char *name,
                   ast_node_t child) {
  hash_map_set(node->children, create_cstring(allocator, name), child, NULL,
               NULL);
  child->parent = node;
}

static void ast_error_dispsoe(ast_error_t self, allocator_t allocator) {
  allocator_free(allocator, self->message);
}

ast_node_t create_ast_error(allocator_t allocator, position_t begin,
                            position_t end, const char *filename,
                            const char *message) {
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_ERROR);
  node->type = NODE_TYPE_ERROR;
  node->error = allocator_alloc(allocator, sizeof(struct _ast_error_t),
                                (dispose_fn_t)ast_error_dispsoe);
  node->error->begin = begin;
  node->error->end = end;
  node->error->filename = filename;
  node->error->message = create_cstring(allocator, message);
  return node;
}

static void ast_doc_dispose(ast_doc_t self, allocator_t allocator) {
  allocator_free(allocator, self->node);
  allocator_free(allocator, self->stream);
  allocator_free(allocator, self->source);
}
static ast_doc_t create_ast_doc(allocator_t allocator, ast_node_t node,
                                token_stream_t stream, char *source) {
  ast_doc_t self = allocator_alloc(allocator, sizeof(struct _ast_doc_t),
                                   (dispose_fn_t)ast_doc_dispose);
  self->node = node;
  self->stream = stream;
  self->source = source;
  return self;
}

ast_doc_t read_ast_node(allocator_t allocator, const char *filename,
                        void *ctx) {
  FILE *fp = fopen(filename, "r");
  if (!fp) {
    size_t len = snprintf(NULL, 0, "failed to open file: %s", filename);
    char buf[len+1];
    sprintf(buf, "failed to open file:%s", filename);
    ast_node_t err = create_ast_error(allocator, (position_t){}, (position_t){},
                                      filename, buf);
    return create_ast_doc(allocator, err, NULL, NULL);
  }
  fseek(fp, 0, SEEK_END);
  size_t len = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  char *source = allocator_alloc(allocator, len + 1, NULL);
  fread(source, len, 1, fp);
  source[len] = 0;
  fclose(fp);
  position_t position = {
      .offset = source,
      .column = 0,
      .line = 0,
  };
  position_t current = position;
  const char *end = source + len;
  token_stream_t stream =
      create_token_stream(allocator, &position, end, filename);
  if (!stream) {
    ast_node_t err = create_ast_error(allocator, position, current, filename,
                                      "unexpected token");
    return create_ast_doc(allocator, err, NULL, NULL);
  } else {
    FILE *fp = fopen("./build/tokens.txt", "w");
    token_t token = NULL;
    while ((token = token_stream_get(stream)) != NULL) {
      switch (token->type) {
      case TOKEN_TYPE_SPACE:
        fprintf(fp, "%-25s", "TOKEN_TYPE_SPACE");
        break;
      case TOKEN_TYPE_COMMENT:
        fprintf(fp, "%-25s", "TOKEN_TYPE_COMMENT");
        break;
      case TOKEN_TYPE_NUMERIC:
        fprintf(fp, "%-25s", "TOKEN_TYPE_NUMERIC");
        break;
      case TOKEN_TYPE_IDENTIFIER:
        fprintf(fp, "%-25s", "TOKEN_TYPE_IDENTIFIER");
        break;
      case TOKEN_TYPE_KEYWORD:
        fprintf(fp, "%-25s", "TOKEN_TYPE_KEYWORD");
        break;
      case TOKEN_TYPE_STRING:
        fprintf(fp, "%-25s", "TOKEN_TYPE_STRING");
        break;
      case TOKEN_TYPE_CHAR:
        fprintf(fp, "%-25s", "TOKEN_TYPE_CHAR");
        break;
      case TOKEN_TYPE_SYMBOL:
        fprintf(fp, "%-25s", "TOKEN_TYPE_SYMBOL");
        break;
      case TOKEN_TYPE_EOF:
        fprintf(fp, "%-25s", "TOKEN_TYPE_EOF");
        break;
      }
      stream->position++;
      fprintf(fp, "| ");
      char *str = location_get(token->loc, allocator);
      char *encode_str = encode_cstring(allocator, str);
      fprintf(fp, "%s", encode_str);
      allocator_free(allocator, encode_str);
      allocator_free(allocator, str);
      fprintf(fp, " |");
      fprintf(fp, "\n");
    }
    fclose(fp);
  }
  stream->position = 0;
  ast_node_t program = read_program(allocator, stream);
  return create_ast_doc(allocator, program, stream, source);
}
ast_node_t clone_ast_node(allocator_t allocator, ast_node_t node) {
  if (node->type == NODE_TYPE_ERROR) {
    return create_ast_error(allocator, node->error->begin, node->error->end,
                            node->error->filename, node->error->message);
  }
  ast_node_t n = create_ast_node(allocator, node->type);
  n->start = node->start;
  n->end = node->end;
  n->filename = node->filename;
  if (node->type == NODE_TYPE_LIST) {
    for (size_t idx = 0; idx < ast_get_length(node); idx++) {
      ast_node_t item = ast_get_item(node, idx);
      item = clone_ast_node(allocator, item);
      ast_add_item(n, item);
    }
  } else if (node->type > NODE_TYPE_LIST) {
    for (list_node_t it = hash_map_get_first(node->children);
         it != hash_map_get_end(node->children);
         it = hash_map_node_get_next(it)) {
      const char *key = hash_map_node_get_key(it);
      ast_node_t child = hash_map_node_get_value(it);
      child = clone_ast_node(allocator, child);
      ast_add_child(allocator, n, key, child);
    }
  } else if (node->type == NODE_TYPE_VALUE) {
    n->value = value_clone(node->value, allocator);
  }
  return n;
}

ast_node_t create_ast_value(allocator_t allocator, value_t value) {
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_VALUE);
  node->value = value_clone(value, allocator);
  return node;
}
location_t node_get_location(ast_node_t node) {
  return (location_t){
      .begin = node->start->loc.begin,
      .end = node->end->loc.begin,
      .filename = node->filename,
  };
}
bool node_location_is(ast_node_t node, const char *src) {
  location_t loc = node_get_location(node);
  return location_is(loc, src);
}