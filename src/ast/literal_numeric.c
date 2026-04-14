#include "ast/literal_numeric.h"
#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"

cubec_ast_node_t cubec_read_ast_literal_numeric(cubec_allocator_t allocator,
                                                cubec_position_t *position,
                                                const char *end,
                                                const char *filename) {
  if (*position->offset < '0' || *position->offset > '9') {
    return NULL;
  }
  cubec_ast_node_t node =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_LITERAL_NUMERIC);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  if (*current.offset == '0' &&
      (*(current.offset + 1) == 'x' || *(current.offset + 1) == 'X')) {
    current.offset += 2;
    current.column += 2;
    if (!(*current.offset >= '0' && *current.offset <= '9') ||
        (*current.offset >= 'a' && *current.offset <= 'f') ||
        (*current.offset >= 'A' && *current.offset <= 'F')) {
      err = cubec_create_ast_error(allocator, *position, current, filename,
                                   "Invalid or unexpected token");
      goto onerror;
    }
    while ((*current.offset >= '0' && *current.offset <= '9') ||
           (*current.offset >= 'a' && *current.offset <= 'f') ||
           (*current.offset >= 'A' && *current.offset <= 'F') ||
           *current.offset == '_') {
      current.offset++;
      current.column++;
    }
    if (*(current.offset - 1) == '_') {
      err = cubec_create_ast_error(allocator, *position, current, filename,
                                   "Invalid or unexpected token");
      goto onerror;
    }
  } else if (*current.offset == '0' &&
             (*(current.offset + 1) == 'o' || *(current.offset + 1) == 'O')) {
    current.offset += 2;
    current.column += 2;
    if (!(*current.offset >= '0' && *current.offset <= '7')) {
      err = cubec_create_ast_error(allocator, *position, current, filename,
                                   "Invalid or unexpected token");
      goto onerror;
    }
    while ((*current.offset >= '0' && *current.offset <= '7') ||
           *current.offset == '_') {
      current.offset++;
      current.column++;
    }
    if (*(current.offset - 1) == '_') {
      err = cubec_create_ast_error(allocator, *position, current, filename,
                                   "Invalid or unexpected token");
      goto onerror;
    }
  } else if (*current.offset == '0' &&
             (*(current.offset + 1) == 'b' || *(current.offset + 1) == 'B')) {
    current.offset += 2;
    current.column += 2;
    if (!(*current.offset >= '0' && *current.offset <= '1')) {
      err = cubec_create_ast_error(allocator, *position, current, filename,
                                   "Invalid or unexpected token");
      goto onerror;
    }
    while ((*current.offset >= '0' && *current.offset <= '1') ||
           *current.offset == '_') {
      current.offset++;
      current.column++;
    }
    if (*(current.offset - 1) == '_') {
      err = cubec_create_ast_error(allocator, *position, current, filename,
                                   "Invalid or unexpected token");
      goto onerror;
    }
  } else if (*current.offset >= '0' && *current.offset <= '9' ||
             *current.offset == '.') {
    if (*current.offset >= '0' && *current.offset <= '9') {
      while (*current.offset >= '0' && *current.offset <= '9' ||
             *current.offset == '_') {
        current.offset++;
        current.column++;
      }
      if (*(current.offset - 1) == '_') {
        err = cubec_create_ast_error(allocator, *position, current, filename,
                                     "Invalid or unexpected token");
        goto onerror;
      }
    }
    if (*current.offset == '.') {
      current.offset++;
      current.column++;
      if ((*current.offset < '0' || *current.offset > '9') &&
          *position->offset == '.') {
        err = cubec_create_ast_error(allocator, *position, current, filename,
                                     "Invalid or unexpected token");
        goto onerror;
      }
      while (*current.offset >= '0' && *current.offset <= '9' ||
             *current.offset == '_') {
        current.offset++;
        current.column++;
      }
      if (*(current.offset - 1) == '_') {
        err = cubec_create_ast_error(allocator, *position, current, filename,
                                     "Invalid or unexpected token");
        goto onerror;
      }
    }
    if (*current.offset == 'e' || *current.offset == 'E') {
      current.offset++;
      current.column++;
      if (*current.offset < '0' || *current.offset > '9') {
        err = cubec_create_ast_error(allocator, *position, current, filename,
                                     "Invalid or unexpected token");
        goto onerror;
      }
      while (*current.offset >= '0' && *current.offset <= '9' ||
             *current.offset == '_') {
        current.offset++;
        current.column++;
      }
      if (*(current.offset - 1) == '_') {
        err = cubec_create_ast_error(allocator, *position, current, filename,
                                     "Invalid or unexpected token");
        goto onerror;
      }
    }
  } else {
    goto onerror;
  }

  cubec_ast_node_t value =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_LITERAL_NUMERIC_VALUE);
  value->loc.begin = *position;
  value->loc.end = current;
  value->loc.filename = filename;
  cubec_ast_add_child(allocator, node, "value", value);
  if (*current.offset == '@') {
    current.offset++;
    current.column++;
    cubec_ast_node_t type =
        cubec_read_ast_literal_identifier(allocator, &current, end, filename);
    if (type) {
      cubec_ast_add_child(allocator, node, "type", type);
    }
  }
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;
  return node;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}