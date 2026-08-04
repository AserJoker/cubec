#include "cubec/statement_test.h"
#include "core/emit_context.h"
#include "core/string.h"
#include "core/token.h"
#include "core/writer.h"
#include "cubec/node_error.h"
#include "cubec/statement_block.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_statement_test_init(cubec_statement_test_t self,
                                       allocator_t allocator,
                                       cubec_statement_test_init_t *init) {
  if (!init)
    return;
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_TEST,
      .parent = NULL,
  };
  super_init.location = init->location;
  g_node_type.init(&self->super, allocator, &super_init);
  self->name = init->name;
  self->body = init->body;
}

static void _cubec_statement_test_dispose(cubec_statement_test_t self,
                                          allocator_t allocator) {
  allocator_free(allocator, &self->body);
  allocator_free(allocator, &self->name);
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_statement_test_clone(cubec_statement_test_t self,
                                        allocator_t allocator,
                                        cubec_statement_test_t another) {
  g_node_type.clone(&self->super, allocator, &another->super);
  self->name = (string_t)value_clone(allocator, another->name);
  self->body = value_clone(allocator, another->body);
  return;
}

static void _cubec_statement_test_move(cubec_statement_test_t self,
                                       allocator_t allocator,
                                       cubec_statement_test_t another) {
  g_node_type.move(&self->super, allocator, &another->super);
  self->name = (string_t)value_move(allocator, another->name);
  self->body = value_move(allocator, another->body);
  return;
}

type_t g_cubec_statement_test_type = {
    .name = "cubec.cubec.statement_test",
    .size = sizeof(struct _cubec_statement_test_t),
    .init = (type_init_fn_t)_cubec_statement_test_init,
    .dispose = (type_dispose_fn_t)_cubec_statement_test_dispose,
    .clone = (type_clone_fn_t)_cubec_statement_test_clone,
    .move = (type_move_fn_t)_cubec_statement_test_move,
};

/* --------------------------------------------------------------------------
 *  Helper: check keyword / symbol
 * -------------------------------------------------------------------------- */

static bool _is_keyword(vec_t tokens, size_t position, const char *keyword) {
  token_t token = vec_get(tokens, position);
  if (!token)
    return false;
  if (token_get_kind(token) != CUBEC_TOKEN_KEYWORD)
    return false;
  return location_is(token_get_location(token), keyword);
}

/* --------------------------------------------------------------------------
 *  Parser: read_statement_test — test "name" { }
 * -------------------------------------------------------------------------- */

node_t read_statement_test(context_t ctx, vec_t tokens, size_t *position,
                           const char *filename) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;
  string_t name = NULL;
  node_t body = NULL;
  cubec_statement_test_t node = NULL;

  /* 1. Expect 'test' keyword */
  if (!_is_keyword(tokens, current, "test")) {
    return NULL;
  }
  token_t test_token = vec_get(tokens, current);
  location_t start_location = *token_get_location(test_token);
  start_location.filename = filename;
  current++;
  skip_whitespace(tokens, &current);

  /* 2. Expect string literal for name */
  token_t name_token = vec_get(tokens, current);
  if (token_get_kind(name_token) != CUBEC_TOKEN_STRING) {
    goto onerror;
  }
  name = allocator_create(allocator, &g_string_type, NULL);
  string_nconcat(name, token_get_string(name_token),
                 token_get_string_length(name_token));
  current++;
  skip_whitespace(tokens, &current);

  /* 3. Parse body (block) */
  body = read_statement_block(ctx, tokens, &current, filename);
  if (node_is_error(body)) {
    allocator_free(allocator, &name);
    return body;
  }
  if (!body)
    goto onerror;

  /* 4. Build location */
  location_t loc = start_location;
  loc.end = body->location.end;

  cubec_statement_test_init_t init = {
      .location = loc,
      .parent = NULL,
      .name = name,
      .body = body,
  };
  node = allocator_create(allocator, &g_cubec_statement_test_type, &init);
  *position = current;
  return &node->super;

onerror:
  allocator_free(allocator, &body);
  allocator_free(allocator, &name);
  allocator_free(allocator, &node);
  return create_error(ctx, start_location);
}

node_t create_statement_test(context_t ctx, location_t loc, const char *name,
                             node_t body) {
  allocator_t alloc = ctx->allocator;
  string_t name_str =
      allocator_create(alloc, &g_string_type, &(string_init_t){.str = name});
  cubec_statement_test_init_t init = {
      .location = loc, .parent = NULL, .name = name_str, .body = body};
  return (node_t)allocator_create(alloc, &g_cubec_statement_test_type, &init);
}

void write_statement_test(writer_t writer, node_t stmt) {
  cubec_statement_test_t test = (cubec_statement_test_t)stmt;
  writer_append(writer, "test ");
  writer_append(writer, string_get(test->name));
  writer_append(writer, " ");
  write_statement_block(writer, test->body);
}

void emit_statement_test(emit_context_t ctx, node_t stmt) {
  cubec_statement_test_t test = (cubec_statement_test_t)stmt;
  recover_comments_to(ctx, stmt->location.begin.offset);
  emit_keyword(ctx, "test");
  emit_space(ctx);
  emit_string_literal(ctx, string_get(test->name));
  emit_space(ctx);
  emit_statement_block(ctx, test->body);
}