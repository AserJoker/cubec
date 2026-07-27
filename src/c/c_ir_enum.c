#include "c/c_ir_enum.h"

c_ir_enum_item_t c_ir_enum_item_create(allocator_t allocator, const char *name,
                                          const char *value) {
  c_ir_enum_item_t item = allocator_alloc(allocator, sizeof(struct _c_ir_enum_item_t));
  item->name = allocator_create(allocator, &g_string_type,
                                 &(string_init_t){.str = name});
  item->value = allocator_create(allocator, &g_string_type,
                                  &(string_init_t){.str = value});
  return item;
}

void c_ir_enum_item_dispose(allocator_t allocator, c_ir_enum_item_t *item) {
  if (!item || !*item) return;
  c_ir_enum_item_t i = *item;
  allocator_free(allocator, &i->name);
  allocator_free(allocator, &i->value);
  allocator_free(allocator, item);
}

c_ir_enum_def_t c_ir_enum_def_create(allocator_t allocator, const char *name,
                                       c_type_t backing_type, vec_t items,
                                       location_t source_loc) {
  c_ir_enum_def_t node = allocator_alloc(allocator, sizeof(struct _c_ir_enum_def_t));
  node->kind = C_IR_ENUM_DEF;
  node->source_loc = source_loc;
  node->name = allocator_create(allocator, &g_string_type,
                                 &(string_init_t){.str = name});
  node->backing_type = backing_type;
  node->items = items;
  return node;
}

void c_ir_enum_def_dispose(allocator_t allocator, c_ir_enum_def_t *node) {
  if (!node || !*node) return;
  c_ir_enum_def_t n = *node;
  allocator_free(allocator, &n->name);
  if (n->backing_type) c_type_dispose(allocator, &n->backing_type);
  /* Dispose each enum item in the items vec */
  if (n->items) {
    size_t count = vec_get_size(n->items);
    for (size_t i = 0; i < count; i++) {
      c_ir_enum_item_t item = vec_get(n->items, i);
      c_ir_enum_item_dispose(allocator, &item);
    }
    allocator_free(allocator, &n->items);
  }
  allocator_free(allocator, node);
}
