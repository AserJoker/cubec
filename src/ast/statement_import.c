#include "ast/statement_import.h"
#include "ast/import_declarator.h"
#include "ast/literal_identifier.h"
#include "ast/literal_string.h"
#include "ast/literal_symbol.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"
static void
cubec_ast_statement_import_dispose(cubec_ast_statement_import_t self,
                                   cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->declarators);
  cubec_allocator_free(allocator, self->source);
  cubec_ast_node_dispose(allocator, &self->super);
}
cubec_ast_statement_import_t
cubec_create_ast_statement_import(cubec_allocator_t allocator) {
  cubec_ast_statement_import_t statement = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_ast_statement_import_t),
      (cubec_dispose_fn_t)cubec_ast_statement_import_dispose);
  cubec_ast_node_initialize(allocator, &statement->super);
  statement->super.type = CUBEC_NODE_TYPE_STATEMENT_IMPORT;
  cubec_ast_set_field(statement, allocator, source);
  cubec_ast_set_field(statement, allocator, declarators);
  statement->declarators = cubec_create_ast_list_node(allocator);
  return statement;
}
cubec_ast_node_t cubec_read_ast_statement_import(cubec_allocator_t allocator,
                                                 cubec_position_t *position,
                                                 const char *end) {
  cubec_position_t current = *position;
  cubec_ast_statement_import_t node = NULL;
  cubec_ast_node_t err = NULL;
  cubec_ast_node_t identifier =
      cubec_read_ast_literal_identifier(allocator, &current, end);
  if (!identifier) {
    return NULL;
  }
  if (identifier->type == CUBEC_NODE_TYPE_ERROR) {
    err = identifier;
    goto onerror;
  }
  if (!cubec_location_is(identifier->loc, "import")) {
    cubec_allocator_free(allocator, identifier);
    return NULL;
  }
  cubec_allocator_free(allocator, identifier);
  node = cubec_create_ast_statement_import(allocator);
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  cubec_allocator_free(allocator, err);
  cubec_ast_node_t token =
      cubec_read_ast_literal_symbol(allocator, &current, end);
  if (!token) {
    token = cubec_read_ast_literal_identifier(allocator, &current, end);
  }
  if (!token) {
    token = cubec_read_ast_literal_string(allocator, &current, end);
  }
  if (!token) {
    err = cubec_create_ast_error(
        allocator, *position, current,
        "Invalid import statement, missing import declarator");
    goto onerror;
  }
  if (token->type == CUBEC_NODE_TYPE_ERROR) {
    err = token;
    goto onerror;
  }
  if (token->type == CUBEC_NODE_TYPE_LITERAL_STRING) {
    node->source = token;
  } else {
    if (token->type == CUBEC_NODE_TYPE_LITERAL_SYMBOL) {
      if (cubec_location_is(token->loc, "*")) {
        current = token->loc.begin;
        cubec_allocator_free(allocator, token);
        cubec_ast_node_t declarator =
            cubec_read_ast_import_namespace(allocator, &current, end);
        if (!declarator) {
          cubec_allocator_free(allocator, token);
          err = cubec_create_ast_error(
              allocator, *position, current,
              "Invalid import namespace, unexcept token");
          return err;
        }
        if (declarator->type == CUBEC_NODE_TYPE_ERROR) {
          err = declarator;
          goto onerror;
        }
        cubec_ast_list_node_append(node->declarators, allocator, declarator);
      } else if (cubec_location_is(token->loc, "{")) {
        cubec_allocator_free(allocator, token);
        err = cubec_ast_skip_all(allocator, &current, end);
        if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
          goto onerror;
        }
        cubec_allocator_free(allocator, err);

        if (*current.offset != '}') {
          for (;;) {
            cubec_ast_node_t declarator =
                cubec_read_ast_import_declarator(allocator, &current, end);
            if (!declarator) {
              err = cubec_create_ast_error(
                  allocator, *position, current,
                  "Invalid import statement, unexcept token");
              goto onerror;
            }
            if (declarator->type == CUBEC_NODE_TYPE_ERROR) {
              err = declarator;
              goto onerror;
            }
            cubec_ast_list_node_append(node->declarators, allocator,
                                       declarator);
            err = cubec_ast_skip_all(allocator, &current, end);
            if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
              goto onerror;
            }
            cubec_allocator_free(allocator, err);
            if (*current.offset == '}') {
              break;
            }
            if (*current.offset != ',') {
              err = cubec_create_ast_error(
                  allocator, *position, current,
                  "Invalid import statement, unexcept token");
              goto onerror;
            }
          }
        }
        err = cubec_ast_skip_all(allocator, &current, end);
        if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
          goto onerror;
        }
        cubec_allocator_free(allocator, err);
        token = cubec_read_ast_literal_symbol(allocator, &current, end);
        if (!token) {
          err = cubec_create_ast_error(
              allocator, *position, current,
              "Invalid import statement, missing symbol '}'");
          goto onerror;
        }
        if (token->type == CUBEC_NODE_TYPE_ERROR) {
          err = token;
          goto onerror;
        }
        cubec_allocator_free(allocator, token);
      } else {
        cubec_allocator_free(allocator, token);
        err =
            cubec_create_ast_error(allocator, *position, current,
                                   "Invalid import statement, unexcept symbol");
        goto onerror;
      }
    } else if (token->type == CUBEC_NODE_TYPE_LITERAL_IDENTIFIER) {
      cubec_ast_import_declarator declarator =
          cubec_create_ast_import_declarator(allocator);
      declarator->alias = token;
      declarator->super.loc = token->loc;
    } else {
      cubec_allocator_free(allocator, token);
      err = cubec_create_ast_error(allocator, *position, current,
                                   "Invalid import statement, unexcept symbol");
      goto onerror;
    }
    err = cubec_ast_skip_all(allocator, &current, end);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      goto onerror;
    }
    cubec_allocator_free(allocator, err);
    token = cubec_read_ast_literal_identifier(allocator, &current, end);
    if (!token) {
      err = cubec_create_ast_error(allocator, *position, current,
                                   "Invalid import statement, missing 'from'");
      goto onerror;
    }
    if (token->type == CUBEC_NODE_TYPE_ERROR) {
      return token;
    }
    if (!cubec_location_is(token->loc, "from")) {
      cubec_allocator_free(allocator, token);
      err = cubec_create_ast_error(allocator, *position, current,
                                   "Invalid import statement, missing 'from'");
      goto onerror;
    }
    cubec_allocator_free(allocator, token);
    err = cubec_ast_skip_all(allocator, &current, end);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      goto onerror;
    }
    cubec_allocator_free(allocator, err);
    token = cubec_read_ast_literal_string(allocator, &current, end);
    if (!token) {
      err = cubec_create_ast_error(allocator, *position, current,
                                   "Invalid import statement, missing source");
      goto onerror;
    }
    if (token->type == CUBEC_NODE_TYPE_ERROR) {
      err = token;
      goto onerror;
    }
    node->source = token;
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  cubec_allocator_free(allocator, err);
  token = cubec_read_ast_literal_symbol(allocator, &current, end);
  if (token->type == CUBEC_NODE_TYPE_ERROR) {
    err = token;
    goto onerror;
  }
  if (!token || !cubec_location_is(token->loc, ";")) {
    cubec_allocator_free(allocator, token);
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid import statement, missing ';'");
    goto onerror;
  }
  cubec_allocator_free(allocator, token);
  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  cubec_ast_set_parent(node->source, &node->super);
  cubec_ast_set_parent(node->declarators, &node->super);
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}