#include "eval/statement_import.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/value.h"

cubec_value_t cubec_eval_statement_import(cubec_context_t ctx,
                                          cubec_ast_node_t node) {
  cubec_allocator_t allocator = cubec_context_get_allocator(ctx);

  cubec_ast_node_t identifier = cubec_ast_get_child(node, "identifier");
  char *c_id = cubec_location_get(identifier->loc, allocator);
  cubec_ast_node_t source = cubec_ast_get_child(node, "source");
  char *c_source = cubec_location_get(source->loc, allocator);
  cubec_value_t value = cubec_context_load_module(ctx, c_source);
  if (cubec_value_is_error(value)) {
    const char *msg = *(const char **)cubec_value_get_data(value);
    cubec_value_t err = cubec_create_compile_error(
        ctx, node, "Failed to load module '%s', caused by\n %s", c_source, msg);
    cubec_allocator_free(allocator, c_source);
    cubec_allocator_free(allocator, c_id);
    return err;
  }
  value = cubec_value_clone(allocator, value);
  value = cubec_context_declar(ctx, c_id, value);
  cubec_allocator_free(allocator, c_source);
  cubec_allocator_free(allocator, c_id);
  return value;
}