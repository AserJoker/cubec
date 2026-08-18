#include "cubec/union_field.h"
#include "core/emit_context.h"
#include "core/token.h"
#include "core/token_writer.h"
#include "cubec/literal_identifier.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_union_field_init(cubec_union_field_t self,
                                    allocator_t allocator,
                                    cubec_union_field_init_t *init) {
  if (!init)
    return;
  node_init_t super_init = {
      .kind = CUBEC_NODE_UNION_FIELD,
      .parent = NULL,
  };
  super_init.location = init->location;
  g_node_class.init(&self->super, allocator, &super_init);
  self->name = init->name;
  self->type = init->type;
}

static void _cubec_union_field_dispose(cubec_union_field_t self,
                                       allocator_t allocator) {
  allocator_free(allocator, &self->type);
  allocator_free(allocator, &self->name);
  g_node_class.dispose(&self->super, allocator);
}

static void _cubec_union_field_clone(cubec_union_field_t self,
                                     allocator_t allocator,
                                     cubec_union_field_t another) {
  g_node_class.clone(&self->super, allocator, &another->super);
  self->name = alloc_clone(allocator, another->name);
  self->type = alloc_clone(allocator, another->type);
  return;
}

static void _cubec_union_field_move(cubec_union_field_t self,
                                    allocator_t allocator,
                                    cubec_union_field_t another) {
  g_node_class.move(&self->super, allocator, &another->super);
  self->name = alloc_move(allocator, another->name);
  self->type = alloc_move(allocator, another->type);
  return;
}

class_t g_cubec_union_field_class = {
    .name = "cubec.cubec.union_field",
    .size = sizeof(struct _cubec_union_field_t),
    .init = (class_init_fn_t)_cubec_union_field_init,
    .dispose = (class_dispose_fn_t)_cubec_union_field_dispose,
    .clone = (class_clone_fn_t)_cubec_union_field_clone,
    .move = (class_move_fn_t)_cubec_union_field_move,
};

/* --------------------------------------------------------------------------
 *  Parser: read_union_field — <identifier> : <type> ;
 * -------------------------------------------------------------------------- */

node_t read_union_field(vm_t vm, vec_t tokens, size_t *position,
                        const char *filename) {
  allocator_t allocator = vm_get_allocator(vm);
  size_t current = *position;
  node_t name = NULL;
  node_t type_expr = NULL;
  cubec_union_field_t node = NULL;

  /* Parse field name (identifier) */
  name = read_literal_identifier(vm, tokens, &current, filename);
  if (!name) {
    goto cleanup;
  }

  /* Expect ':' */
  skip_whitespace(tokens, &current);
  token_t colon_token = vec_get(tokens, current);
  if (!token_is(colon_token, CUBEC_TOKEN_SYMBOL, ":")) {
    goto cleanup;
  }
  (void)0; /* colon consumed */
  current++;
  skip_whitespace(tokens, &current);

  /* Parse type expression */
  type_expr = read_expression_type(vm, tokens, &current, filename);
  if (!type_expr) {
    goto cleanup;
  }

  /* Expect ';' */
  skip_whitespace(tokens, &current);
  token_t semi = vec_get(tokens, current);
  if (!token_is(semi, CUBEC_TOKEN_SYMBOL, ";")) {
    goto cleanup;
  }
  current++;

  /* Build location from name start to ';' end */
  location_t start_loc = name->location;
  location_t *end_loc = token_get_location(semi);
  location_t loc = {
      .begin = start_loc.begin,
      .end = end_loc->end,
      .filename = filename,
  };

  cubec_union_field_init_t init = {
      .location = loc,
      .parent = NULL,
      .name = name,
      .type = type_expr,
  };
  node = allocator_create(allocator, &g_cubec_union_field_class, &init);
  if (!node)
    goto cleanup;
  *position = current;
  return &node->super;

cleanup:
  allocator_free(allocator, &type_expr);
  allocator_free(allocator, &name);
  allocator_free(allocator, &node);
  return NULL;
}

/* --------------------------------------------------------------------------
 *  Factory: create_union_field
 * -------------------------------------------------------------------------- */

node_t create_union_field(vm_t vm, location_t loc, const char *name,
                          node_t type) {
  allocator_t alloc = vm_get_allocator(vm);
  node_t name_node = create_literal_identifier(vm, loc, name);
  cubec_union_field_init_t init = {
      .location = loc,
      .parent = NULL,
      .name = name_node,
      .type = type,
  };
  return (node_t)allocator_create(alloc, &g_cubec_union_field_class, &init);
}

void emit_union_field(emit_context_t ctx, node_t node) {
  cubec_union_field_t field = (cubec_union_field_t)node;
  recover_comments_to(ctx, node->location.begin.offset);
  emit_expression(ctx, field->name);
  emit_symbol(ctx, ":");
  emit_space(ctx);
  emit_expression(ctx, field->type);
  emit_symbol(ctx, ";");
}
