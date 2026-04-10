#include "eval/ptr_declarator.h"
#include "ast/node.h"
#include "core/location.h"
#include "engine/error.h"
#include "engine/ptr.h"
#include "engine/type.h"
#include "engine/value.h"
#include "eval/type.h"
cubec_value_t cubec_eval_ptr_declarator(cubec_context_t ctx,
                                        cubec_ast_node_t node) {
  cubec_ast_node_t type_node = cubec_ast_get_child(node, "type");
  cubec_ast_node_t kind = cubec_ast_get_child(node, "kind");
  cubec_ast_node_t decorators = cubec_ast_get_child(node, "decorators");
  cubec_value_t vtype = cubec_eval_type(ctx, type_node);
  if (cubec_value_is_error(vtype)) {
    return vtype;
  }
  cubec_type_t type = *(cubec_type_t *)cubec_value_get_data(vtype);
  bool is_mutable = true;
  bool is_volatile = false;
  for (size_t idx = 0; idx < cubec_ast_get_length(decorators); idx++) {
    cubec_ast_node_t dec = cubec_ast_get_item(decorators, idx);
    if (cubec_location_is(dec->loc, "const")) {
      is_mutable = false;
    }
    if (cubec_location_is(dec->loc, "volatile")) {
      is_volatile = true;
    }
  }
  if (cubec_location_is(kind->loc, "*")) {
    return cubec_create_ptr_type(ctx, type, is_mutable, is_volatile);
  } else if (cubec_location_is(kind->loc, "[*]")) {
    return cubec_create_ptr_array_type(ctx, type, is_mutable, is_volatile);
  } else {
    return cubec_create_compile_error(ctx, node, "unsupport ptr type");
  }
}