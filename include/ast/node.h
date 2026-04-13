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
struct _cubec_ast_node_t;
typedef struct _cubec_ast_node_t *cubec_ast_node_t;
struct _cubec_ast_node_t {
  cubec_location_t loc;
  cubec_ast_node_type_t type;
  cubec_ast_node_t parent;
  bool changed;
  union {
    cubec_array_t items;
    cubec_hash_map_t children;
  };
};

cubec_ast_node_t cubec_create_ast_node(cubec_allocator_t allocator,
                                       size_t type);
void cubec_ast_add_child(cubec_allocator_t allocator, cubec_ast_node_t node,
                         const char *name, cubec_ast_node_t child);
void cubec_ast_remove_child(cubec_ast_node_t node, const char *name);
cubec_ast_node_t cubec_ast_move_child(cubec_ast_node_t node, const char *name);
cubec_ast_node_t cubec_ast_get_child(cubec_ast_node_t node, const char *name);
cubec_ast_node_t cubec_ast_get_item(cubec_ast_node_t node, size_t idx);
const char *cubec_ast_get_child_name(cubec_ast_node_t node,
                                     cubec_ast_node_t child);
size_t cubec_ast_get_item_index(cubec_ast_node_t node, cubec_ast_node_t child);
void cubec_ast_add_item(cubec_ast_node_t node, cubec_ast_node_t item);
void cubec_ast_remove_item(cubec_ast_node_t node, size_t idx);
cubec_ast_node_t cubec_ast_move_item(cubec_ast_node_t node, size_t idx);
cubec_ast_node_t cubec_ast_replace_item(cubec_ast_node_t node, size_t idx,
                                        cubec_ast_node_t item);
void cubec_ast_insert_item(cubec_ast_node_t node, size_t pos,
                           cubec_ast_node_t item);
void cubec_ast_set_item(cubec_ast_node_t node, size_t pos,
                        cubec_ast_node_t item);
void cubec_ast_set_child(cubec_allocator_t allocator, cubec_ast_node_t node,
                         const char *name, cubec_ast_node_t child);
size_t cubec_ast_get_length(cubec_ast_node_t node);

int32_t cubec_ast_read_code(cubec_position_t *position, const char *end,
                            const char *filename);

typedef struct _cubec_ast_error_t {
  struct _cubec_ast_node_t super;
  char *message;
} *cubec_ast_error_t;

cubec_ast_node_t cubec_create_ast_error(cubec_allocator_t allocator,
                                        cubec_position_t begin,
                                        cubec_position_t end,
                                        const char *filename,
                                        const char *message);

cubec_ast_node_t cubec_ast_skip_all(cubec_allocator_t allocator,
                                    cubec_position_t *position, const char *end,
                                    const char *filename);
char *cubec_ast_write_json(cubec_allocator_t allocator, cubec_ast_node_t node);

typedef cubec_ast_node_t (*cubec_visit_ast_fn_t)(cubec_allocator_t allocator,
                                                 cubec_ast_node_t node,
                                                 void *ctx);

cubec_ast_node_t cubec_read_ast_node(cubec_allocator_t allocator,
                                     const char *filename, const char *source,
                                     void *ctx, cubec_visit_ast_fn_t visit);
cubec_ast_node_t cubec_clone_ast_node(cubec_allocator_t allocator,
                                      cubec_ast_node_t node);

#ifdef __cplusplus
}
#endif
#endif