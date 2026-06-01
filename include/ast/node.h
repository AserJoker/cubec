#ifndef _H_AST_NODE_
#define _H_AST_NODE_
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/hash_map.h"
#include "core/location.h"
#include "core/position.h"
#include "engine/value.h"
#include "reader/token.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#include <unicode/umachine.h>
#include <unicode/urename.h>
#include <unicode/utf8.h>
#include <unicode/utypes.h>
struct _value_t;
struct _scope_t;
struct _ast_node_t;
typedef struct _ast_node_t *ast_node_t;
typedef struct _ast_error_t *ast_error_t;
struct _ast_error_t {
  const char *filename;
  position_t begin;
  position_t end;
  char *message;
};
struct _ast_node_t {
  const char *filename;
  token_t start;
  token_t end;
  ast_node_type_t type;
  ast_node_t parent;
  struct _scope_t *scope;
  union {
    array_t items;
    hash_map_t children;
    ast_error_t error;
    value_t value;
    void *data;
  };
};
typedef struct _ast_doc_t *ast_doc_t;
struct _ast_doc_t {
  ast_node_t node;
  token_stream_t stream;
  char *source;
};

ast_node_t create_ast_node(allocator_t allocator, size_t type);
void ast_add_child(allocator_t allocator, ast_node_t node, const char *name,
                   ast_node_t child);
void ast_remove_child(ast_node_t node, const char *name);
ast_node_t ast_move_child(ast_node_t node, const char *name);
ast_node_t ast_get_child(ast_node_t node, const char *name);
ast_node_t ast_get_item(ast_node_t node, size_t idx);
const char *ast_get_child_name(ast_node_t node, ast_node_t child);
size_t ast_get_item_index(ast_node_t node, ast_node_t child);
void ast_add_item(ast_node_t node, ast_node_t item);
void ast_remove_item(ast_node_t node, size_t idx);
ast_node_t ast_move_item(ast_node_t node, size_t idx);
ast_node_t ast_replace_item(ast_node_t node, size_t idx, ast_node_t item);
void ast_insert_item(ast_node_t node, size_t pos, ast_node_t item);
void ast_set_item(ast_node_t node, size_t pos, ast_node_t item);
void ast_set_child(allocator_t allocator, ast_node_t node, const char *name,
                   ast_node_t child);
size_t ast_get_length(ast_node_t node);

ast_node_t create_ast_error(allocator_t allocator, position_t begin,
                            position_t end, const char *filename,
                            const char *message);

ast_doc_t read_ast_node(allocator_t allocator, const char *filename, void *ctx);

ast_node_t clone_ast_node(allocator_t allocator, ast_node_t node);

ast_node_t create_ast_value(allocator_t allocator, value_t value);

location_t node_get_location(ast_node_t node);

bool node_location_is(ast_node_t node, const char *src);

#ifdef __cplusplus
}
#endif
#endif