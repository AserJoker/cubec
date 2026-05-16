#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/program.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/compare.h"
#include "core/hash.h"
#include "core/hash_map.h"
#include "core/list.h"
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
ast_node_t create_ast_node_debug(allocator_t allocator, size_t type,
                                 const char *filename, size_t len) {
  ast_node_t node =
      allocator_alloc_debug(allocator, sizeof(struct _ast_node_t),
                            (dispose_fn_t)ast_node_dispose, filename, len);
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

ast_node_t read_ast_node(allocator_t allocator, const char *filename,
                         const char *source, void *ctx) {
  position_t position = {
      .column = 0,
      .line = 0,
      .offset = source,
  };
  position_t current = position;
  const char *end = strlen(source) + source;
  token_stream_t stream =
      create_token_stream(allocator, &position, end, filename);
  if (!stream) {
    return create_ast_error(allocator, position, current, filename,
                            "unexpected token");
  }
  ast_node_t program = read_program(allocator, stream);
  allocator_free(allocator, stream);
  if (program->type == NODE_TYPE_ERROR) {
    return program;
  }
  return program;
}
ast_node_t clone_ast_node(allocator_t allocator, ast_node_t node) {
  if (node->type == NODE_TYPE_ERROR) {
    return create_ast_error(allocator, node->error->begin, node->error->end,
                            node->error->filename, node->error->message);
  }
  ast_node_t n = create_ast_node(allocator, node->type);
  n->start = node->start;
  n->end = node->end;
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
      if (strcmp(key, "_value") != 0) {
        ast_node_t child = hash_map_node_get_value(it);
        child = clone_ast_node(allocator, child);
        ast_add_child(allocator, n, key, child);
      }
    }
  }
  return n;
}
