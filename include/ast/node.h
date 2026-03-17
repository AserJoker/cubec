#ifndef _H_CUBEC_AST_NODE_
#define _H_CUBEC_AST_NODE_
#include "core/allocator.h"
#include "core/list.h"
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
  cubec_map_t meta;
  cubec_ast_node_t parent;
};

void cubec_ast_node_initialize(cubec_allocator_t allocator,
                               cubec_ast_node_t self);

void cubec_ast_node_dispose(cubec_allocator_t allocator, cubec_ast_node_t self);

int32_t cubec_ast_read_code(cubec_position_t *position, const char *end);

void cubec_ast_init_field(cubec_ast_node_t self, cubec_allocator_t allocator,
                          const char *name, cubec_ast_node_t *field);
#define cubec_ast_set_field(self, allocator, field)                            \
  cubec_ast_init_field(&self->super, allocator, #field, &self->field)
void cubec_ast_set_parent(cubec_ast_node_t node, cubec_ast_node_t parent);

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

typedef struct _cubec_ast_list_node_t {
  struct _cubec_ast_node_t super;
  cubec_list_t items;
} *cubec_ast_list_node_t;

cubec_ast_node_t cubec_create_ast_list_node(cubec_allocator_t allocator);
void cubec_ast_list_node_append(cubec_ast_node_t self,
                                cubec_allocator_t allocator,
                                cubec_ast_node_t item);

typedef cubec_ast_node_t (*cubec_ast_visit_fn_t)(cubec_ast_node_t node,
                                                 cubec_allocator_t allocator,
                                                 void *arg);

cubec_ast_node_t cubec_visit_node(cubec_ast_node_t node,
                                  cubec_allocator_t allocator,
                                  cubec_ast_visit_fn_t visit, void *arg);

char *cubec_ast_write_json(cubec_allocator_t allocator, cubec_ast_node_t node);

#ifdef __cplusplus
}
#endif
#endif