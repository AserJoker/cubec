#ifndef _H_CUBEC_AST_NODE_
#define _H_CUBEC_AST_NODE_
#include "core/allocator.h"
#include "core/array.h"
#include "core/map.h"
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
  size_t type;
  cubec_ast_node_t parent;
  union {
    cubec_array_t items;
    cubec_map_t children;
  };
};

cubec_ast_node_t cubec_create_ast_node(cubec_allocator_t allocator,
                                       size_t type);
void cubec_ast_add_child(cubec_allocator_t allocator, cubec_ast_node_t node,
                         const char *name, cubec_ast_node_t child);
void cubec_ast_add_item(cubec_allocator_t allocator, cubec_ast_node_t node,
                        cubec_ast_node_t item);

int32_t cubec_ast_read_code(cubec_position_t *position, const char *end);

typedef struct _cubec_ast_error_t {
  struct _cubec_ast_node_t super;
  char *message;
} *cubec_ast_error_t;

cubec_ast_node_t cubec_create_ast_error(cubec_allocator_t allocator,
                                        cubec_position_t begin,
                                        cubec_position_t end,
                                        const char *message);

cubec_ast_node_t cubec_ast_skip_all(cubec_allocator_t allocator,
                                    cubec_position_t *position,
                                    const char *end);
char *cubec_ast_write_json(cubec_allocator_t allocator, cubec_ast_node_t node);

typedef cubec_ast_node_t (*cubec_visit_ast_fn_t)(cubec_allocator_t allocator,
                                                 cubec_ast_node_t node);

cubec_ast_node_t cubec_read_ast_node(cubec_allocator_t allocator,
                                     const char *source, void *ctx,
                                     size_t num_visits,
                                     cubec_visit_ast_fn_t visits[]);

#ifdef __cplusplus
}
#endif
#endif