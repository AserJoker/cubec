#include "cubec/ast_create_helpers.h"
#include "core/string.h"
#include "cubec/literal_identifier.h"
#include "engine/context.h"

string_t _make_string(context_t ctx, const char *str) {
  allocator_t alloc = ctx->allocator;
  string_init_t si = {.str = str};
  return (string_t)allocator_create(alloc, &g_string_type, &si);
}

cubec_literal_identifier_t _make_ident_node(context_t ctx, location_t loc,
                                            const char *name) {
  allocator_t alloc = ctx->allocator;
  cubec_literal_identifier_init_t init = {
      .location = loc, .parent = NULL, .value = name};
  return (cubec_literal_identifier_t)allocator_create(
      alloc, &g_cubec_literal_identifier_type, &init);
}
