#ifndef _H_CUBEC_NODE_NODE_
#define _H_CUBEC_NODE_NODE_
#include "core/allocator.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#include "ast/node_type.h"
#include "core/location.h"
#include <unicode/umachine.h>
#include <unicode/urename.h>
#include <unicode/utf8.h>
#include <unicode/utypes.h>

typedef struct _cubec_ast_node_t {
  cubec_location_t loc;
  cubec_ast_node_type_t type;
} *cubec_ast_node_t;

void cubec_ast_node_initialize(cubec_allocator_t allocator,
                               cubec_ast_node_t self);

void cubec_ast_node_dispose(cubec_allocator_t allocator, cubec_ast_node_t self);

uint32_t cubec_ast_read_code(cubec_position_t *position, cubec_position_t *end);

typedef struct _cubec_error_t {
  struct _cubec_ast_node_t super;
  char *message;
} *cubec_error_t;

cubec_ast_node_t cubec_create_ast_error(cubec_allocator_t allocator,
                                        cubec_position_t begin,
                                        cubec_position_t end,
                                        const char *message);

cubec_ast_node_t cubec_ast_skip_all(cubec_allocator_t allocator,
                                    cubec_position_t *position,
                                    cubec_position_t *end);

#ifdef __cplusplus
}
#endif
#endif