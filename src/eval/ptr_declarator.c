#include "eval/ptr_declarator.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/array.h"
#include "core/location.h"
#include "core/map.h"
#include "engine/context.h"
#include "engine/type.h"
#include "eval/type.h"
cubec_value_t cubec_eval_ptr_declarator(cubec_context_t ctx,
                                        cubec_ast_node_t ptr,
                                        const char *filename) {
  cubec_ast_node_t type_node = cubec_map_get(ptr->children, "type", NULL);
  cubec_value_t base = cubec_eval_type(ctx, type_node, filename);
  if (base->type->kind == CUBEC_TYPE_KIND_ERROR) {
    return base;
  }
  cubec_type_t base_type = *(cubec_type_t *)base->data;
  bool is_mutable = true;
  bool is_volatile = false;
  bool mask_mutable = false;
  bool mask_volatile = false;
  cubec_ast_node_t decorators =
      cubec_map_get(ptr->children, "decorators", NULL);
  for (size_t idx = 0; idx < cubec_array_get_size(decorators->items); idx++) {
    cubec_ast_node_t dec = cubec_array_get_index(decorators->items, idx);
    if (dec->type != CUBEC_NODE_TYPE_LITERAL_IDENTIFIER) {
      return cubec_context_create_compile_error(ctx, dec, filename,
                                                "Unknown pointer decorator");
    }
    if (cubec_location_is(dec->loc, "const")) {
      if (mask_mutable) {
        return cubec_context_create_compile_error(
            ctx, dec, filename, "Duplicate 'const' declaration specifier");
      }
      mask_mutable = true;
      is_mutable = false;
    } else if (cubec_location_is(dec->loc, "volatile")) {
      if (mask_volatile) {
        return cubec_context_create_compile_error(
            ctx, dec, filename, "Duplicate 'volatile' declaration specifier");
      }
      is_volatile = true;
      mask_volatile = true;
    }
  }
  cubec_type_t type = NULL;
  cubec_ast_node_t kind = cubec_map_get(ptr->children, "kind", NULL);
  if (cubec_location_is(kind->loc, "[*]")) {
    type = cubec_context_create_ptr_array_type(ctx, base_type, is_mutable,
                                               is_volatile);
  } else if (cubec_location_is(kind->loc, "*")) {
    type =
        cubec_context_create_ptr_type(ctx, base_type, is_mutable, is_volatile);
  } else {
    return cubec_context_create_compile_error(ctx, kind, filename,
                                              "Unknown pointer kind");
  }
  return cubec_context_create_type_value(ctx, type, false, NULL);
}