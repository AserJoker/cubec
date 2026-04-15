#ifndef _H_CUBEC_AST_NODE_
#define _H_CUBEC_AST_NODE_
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/hash_map.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#include "core/location.h"
#include <unicode/umachine.h>
#include <unicode/urename.h>
#include <unicode/utf8.h>
#include <unicode/utypes.h>
struct _ast_node_t;
typedef struct _ast_node_t *ast_node_t;
struct _ast_node_t {
  location_t loc;
  ast_node_type_t type;
  ast_node_t parent;
  bool changed;
  union {
    array_t items;
    hash_map_t children;
  };
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

int32_t ast_read_code(position_t *position, const char *end,
                      const char *filename);

typedef struct _ast_error_t {
  struct _ast_node_t super;
  char *message;
} *ast_error_t;

ast_node_t create_ast_error(allocator_t allocator, position_t begin,
                            position_t end, const char *filename,
                            const char *message);

ast_node_t ast_skip_all(allocator_t allocator, position_t *position,
                        const char *end, const char *filename);
char *ast_write_json(allocator_t allocator, ast_node_t node);

ast_node_t read_ast_node(allocator_t allocator, const char *filename,
                         const char *source, void *ctx);
ast_node_t clone_ast_node(allocator_t allocator, ast_node_t node);

#ifdef __cplusplus
}
#endif
#endif