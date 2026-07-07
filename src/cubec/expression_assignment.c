#include "cubec/expression_assignment.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/string.h"
#include "core/token.h"
#include "cubec/expression.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include <inttypes.h>

/* Forward declarations */
extern node_t read_value(allocator_t allocator, vec_t tokens, size_t *position,
                         const char *filename);
extern node_t read_expression(allocator_t allocator, vec_t tokens,
                              size_t *position, const char *filename);

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_expression_assignment_init(
    cubec_expression_assignment_t self, allocator_t allocator,
    cubec_expression_assignment_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  cubec_expression_init_t super_init = {
      .kind = CUBEC_NODE_EXPRESSION_ASSIGNMENT,
      .parent = NULL,
  };
  super_init.location = init->location;
  super_init.parent = init->parent;
  TRY_VOID_LOCAL(onerror, g_cubec_expression_type.init(&self->super, allocator, &super_init));

  self->left = init->lvalue;
  self->right = init->rvalue;
  self->opt = init->opt;
onerror:
  return;
}

static void _cubec_expression_assignment_dispose(
    cubec_expression_assignment_t self, allocator_t allocator) {
  allocator_free(allocator, &self->left);
  allocator_free(allocator, &self->right);
  allocator_free(allocator, &self->opt);
  g_cubec_expression_type.dispose(&self->super, allocator);
}

static void _cubec_expression_assignment_clone(
    cubec_expression_assignment_t self, allocator_t allocator,
    cubec_expression_assignment_t another) {
  TRY_VOID_LOCAL(cleanup, g_cubec_expression_type.clone(&self->super, allocator, &another->super));
  self->left = TRY_LOCAL(cleanup, value_clone(allocator, another->left));
  self->right = TRY_LOCAL(cleanup, value_clone(allocator, another->right));
  self->opt = (string_t)TRY_LOCAL(cleanup, value_clone(allocator, another->opt));
  return;

cleanup:
  /* Clean up already-cloned members on failure */
  allocator_free(allocator, &self->opt);
  allocator_free(allocator, &self->right);
  allocator_free(allocator, &self->left);
}

static void _cubec_expression_assignment_move(
    cubec_expression_assignment_t self, allocator_t allocator,
    cubec_expression_assignment_t another) {
  TRY_VOID_LOCAL(cleanup, g_cubec_expression_type.move(&self->super, allocator, &another->super));
  self->left = TRY_LOCAL(cleanup, value_move(allocator, another->left));
  self->right = TRY_LOCAL(cleanup, value_move(allocator, another->right));
  self->opt = (string_t)TRY_LOCAL(cleanup, value_move(allocator, another->opt));
  return;

cleanup:
  /* Clean up already-moved members on failure */
  allocator_free(allocator, &self->opt);
  allocator_free(allocator, &self->right);
  allocator_free(allocator, &self->left);
}

type_t g_cubec_expression_assignment_type = {
    .name = "cubec.cubec.expression_assignment",
    .size = sizeof(struct _cubec_expression_binary_t),
    .init = (type_init_fn_t)_cubec_expression_assignment_init,
    .dispose = (type_dispose_fn_t)_cubec_expression_assignment_dispose,
    .clone = (type_clone_fn_t)_cubec_expression_assignment_clone,
    .move = (type_move_fn_t)_cubec_expression_assignment_move,
};

/* --------------------------------------------------------------------------
 *  Parser: read_expression_assignment
 * -------------------------------------------------------------------------- */

/** Set of supported assignment operator strings (sorted by length descending) */
static const char *assignment_operators[] = {
    "<<=", ">>=", "&&=", "||=",  // 3-char operators first
    "+=",  "-=",  "*=",  "/=",  "%=",  "&=",  "|=",  "^=",  "=",
};

static bool is_assignment_operator_token(token_t tok) {
  if (token_get_kind(tok) != CUBEC_TOKEN_SYMBOL) {
    return false;
  }
  for (size_t i = 0;
       i < sizeof(assignment_operators) / sizeof(assignment_operators[0]);
       i++) {
    if (token_is(tok, CUBEC_TOKEN_SYMBOL, assignment_operators[i])) {
      return true;
    }
  }
  return false;
}

node_t read_expression_assignment(allocator_t allocator, vec_t tokens,
                                  size_t *position, const char *filename) {
  size_t current = *position;
  node_t lvalue = NULL;
  node_t rvalue = NULL;
  cubec_expression_assignment_t node = NULL;
  string_t opt = NULL;
  token_t op_token = NULL;

  /* First, read a value as the potential lvalue */
  lvalue = TRY_LOCAL(onerror, read_value(allocator, tokens, &current, filename));
  if (!lvalue) {
    return NULL;
  }

  /* Skip whitespace and check for assignment operator */
  skip_whitespace(tokens, &current);
  token_t tok = vec_get(tokens, current);

  if (!is_assignment_operator_token(tok)) {
    /* Not an assignment expression — discard lvalue and return NULL.
     * The lvalue may be part of a larger expression (e.g., "a + b")
     * that should be parsed by read_expression_binary. */
    allocator_free(allocator, &lvalue);
    return NULL;
  }

  /* This is an assignment expression — committed from here */
  op_token = tok;
  const char *op_text = token_get_string(op_token);
  size_t op_len = token_get_string_length(op_token);
  opt = TRY_LOCAL(onerror, allocator_create(allocator, &g_string_type, NULL));
  string_nconcat(opt, op_text, op_len);
  current++; /* consume operator */

  /* Parse rvalue expression */
  skip_whitespace(tokens, &current);
  rvalue = TRY_LOCAL(onerror,
                     read_expression(allocator, tokens, &current, filename));
  if (!rvalue) {
    /* No rvalue expression found — this is an error */
    THROW_LOCAL(onerror, "%s:%" PRIuPTR ":%" PRIuPTR " expected expression after assignment operator",
                filename, token_get_location(op_token)->end.line + 1,
                token_get_location(op_token)->end.column + 1);
  }

  /* Create assignment node */
  node = TRY_LOCAL(onerror, allocator_create(allocator, &g_cubec_expression_assignment_type,
                          &(cubec_expression_assignment_init_t){
                              .lvalue = lvalue,
                              .rvalue = rvalue,
                              .opt = opt,
                          }));

  /* Location spans from lvalue start to rvalue end */
  {
    location_t loc = lvalue->location;
    loc.end = rvalue->location.end;
    node->super.super.location = loc;
    node->super.super.location.filename = filename;
  }

  *position = current;
  return (node_t)node;

onerror:
  allocator_free(allocator, &opt);
  allocator_free(allocator, &rvalue);
  allocator_free(allocator, &lvalue);
  allocator_free(allocator, &node);
  return NULL;
}