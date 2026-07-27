#include "c/c_ir_stmt_jump.h"

c_ir_stmt_break_t c_ir_stmt_break_create(allocator_t allocator,
                                           location_t source_loc) {
  c_ir_stmt_break_t node = allocator_alloc(allocator, sizeof(struct _c_ir_stmt_break_t));
  node->kind = C_IR_STMT_BREAK;
  node->source_loc = source_loc;
  return node;
}

void c_ir_stmt_break_dispose(allocator_t allocator, c_ir_stmt_break_t *node) {
  if (!node || !*node) return;
  allocator_free(allocator, node);
}

c_ir_stmt_continue_t c_ir_stmt_continue_create(allocator_t allocator,
                                                  location_t source_loc) {
  c_ir_stmt_continue_t node = allocator_alloc(allocator, sizeof(struct _c_ir_stmt_continue_t));
  node->kind = C_IR_STMT_CONTINUE;
  node->source_loc = source_loc;
  return node;
}

void c_ir_stmt_continue_dispose(allocator_t allocator, c_ir_stmt_continue_t *node) {
  if (!node || !*node) return;
  allocator_free(allocator, node);
}

c_ir_stmt_goto_t c_ir_stmt_goto_create(allocator_t allocator, const char *label,
                                          location_t source_loc) {
  c_ir_stmt_goto_t node = allocator_alloc(allocator, sizeof(struct _c_ir_stmt_goto_t));
  node->kind = C_IR_STMT_GOTO;
  node->source_loc = source_loc;
  node->label = allocator_create(allocator, &g_string_type,
                                  &(string_init_t){.str = label});
  return node;
}

void c_ir_stmt_goto_dispose(allocator_t allocator, c_ir_stmt_goto_t *node) {
  if (!node || !*node) return;
  c_ir_stmt_goto_t n = *node;
  allocator_free(allocator, &n->label);
  allocator_free(allocator, node);
}

c_ir_stmt_label_t c_ir_stmt_label_create(allocator_t allocator, const char *label,
                                            location_t source_loc) {
  c_ir_stmt_label_t node = allocator_alloc(allocator, sizeof(struct _c_ir_stmt_label_t));
  node->kind = C_IR_STMT_LABEL;
  node->source_loc = source_loc;
  node->label = allocator_create(allocator, &g_string_type,
                                  &(string_init_t){.str = label});
  return node;
}

void c_ir_stmt_label_dispose(allocator_t allocator, c_ir_stmt_label_t *node) {
  if (!node || !*node) return;
  c_ir_stmt_label_t n = *node;
  allocator_free(allocator, &n->label);
  allocator_free(allocator, node);
}
