#include "c/statement_struct.h"
#include "ast/node.h"
#include "c/function.h"
#include "core/allocator.h"
#include "core/hash_map.h"
#include "core/stream.h"
#include "engine/context.h"
#include "engine/struct.h"
#include "engine/type.h"
static void c_struct(c_writer_t writer, type_t type) {
  hash_map_t attrs = struct_type_get_attributes(type);
  list_node_t it = hash_map_get_first(attrs);
  while (it != hash_map_get_end(attrs)) {
    struct_attribute_t attr = hash_map_node_get_value(it);
    if (attr->type->kind == TYPE_KIND_FUNCTION) {
      c_function_closure(writer, attr->initialize);
    } else if (attr->type->kind == TYPE_KIND_TEMPLATE) {
      c_template_closure(writer, attr->initialize);
    } else if (attr->type->kind == TYPE_KIND_STRUCT) {
      type_t type = *(type_t *)attr->initialize->value;
      c_struct(writer, type);
    }
    it = hash_map_node_get_next(it);
  }
}
void c_statement_struct(c_writer_t writer, ast_node_t node) {
  stream_t stream = writer->stream;
  context_t ctx = writer->ctx;
  allocator_t allocator = ctx->allocator;
  ast_node_t stru = ast_get_child(node, "struct");
  type_t type = *(type_t *)stru->value->data;
  c_struct(writer, type);
}