#include "c/c_ir_expr_literal.h"

/* String literal */
c_ir_expr_string_t c_ir_expr_string_create(allocator_t allocator, const char *value,
                                              location_t source_loc) {
  c_ir_expr_string_t node = allocator_alloc(allocator, sizeof(struct _c_ir_expr_string_t));
  node->kind = C_IR_EXPR_STRING;
  node->source_loc = source_loc;
  node->value = allocator_create(allocator, &g_string_type,
                                  &(string_init_t){.str = value});
  return node;
}

void c_ir_expr_string_dispose(allocator_t allocator, c_ir_expr_string_t *node) {
  if (!node || !*node) return;
  c_ir_expr_string_t n = *node;
  allocator_free(allocator, &n->value);
  allocator_free(allocator, node);
}

/* Numeric literal */
c_ir_expr_numeric_t c_ir_expr_numeric_create(allocator_t allocator, const char *value,
                                                location_t source_loc) {
  c_ir_expr_numeric_t node = allocator_alloc(allocator, sizeof(struct _c_ir_expr_numeric_t));
  node->kind = C_IR_EXPR_NUMERIC;
  node->source_loc = source_loc;
  node->value = allocator_create(allocator, &g_string_type,
                                  &(string_init_t){.str = value});
  return node;
}

void c_ir_expr_numeric_dispose(allocator_t allocator, c_ir_expr_numeric_t *node) {
  if (!node || !*node) return;
  c_ir_expr_numeric_t n = *node;
  allocator_free(allocator, &n->value);
  allocator_free(allocator, node);
}

/* Char literal */
c_ir_expr_char_t c_ir_expr_char_create(allocator_t allocator, const char *value,
                                          location_t source_loc) {
  c_ir_expr_char_t node = allocator_alloc(allocator, sizeof(struct _c_ir_expr_char_t));
  node->kind = C_IR_EXPR_CHAR;
  node->source_loc = source_loc;
  node->value = allocator_create(allocator, &g_string_type,
                                  &(string_init_t){.str = value});
  return node;
}

void c_ir_expr_char_dispose(allocator_t allocator, c_ir_expr_char_t *node) {
  if (!node || !*node) return;
  c_ir_expr_char_t n = *node;
  allocator_free(allocator, &n->value);
  allocator_free(allocator, node);
}

/* Identifier */
c_ir_expr_ident_t c_ir_expr_ident_create(allocator_t allocator, const char *name,
                                            location_t source_loc) {
  c_ir_expr_ident_t node = allocator_alloc(allocator, sizeof(struct _c_ir_expr_ident_t));
  node->kind = C_IR_EXPR_IDENT;
  node->source_loc = source_loc;
  node->name = allocator_create(allocator, &g_string_type,
                                 &(string_init_t){.str = name});
  return node;
}

void c_ir_expr_ident_dispose(allocator_t allocator, c_ir_expr_ident_t *node) {
  if (!node || !*node) return;
  c_ir_expr_ident_t n = *node;
  allocator_free(allocator, &n->name);
  allocator_free(allocator, node);
}

/* NULL */
c_ir_expr_null_t c_ir_expr_null_create(allocator_t allocator, location_t source_loc) {
  c_ir_expr_null_t node = allocator_alloc(allocator, sizeof(struct _c_ir_expr_null_t));
  node->kind = C_IR_EXPR_NULL;
  node->source_loc = source_loc;
  return node;
}

void c_ir_expr_null_dispose(allocator_t allocator, c_ir_expr_null_t *node) {
  if (!node || !*node) return;
  allocator_free(allocator, node);
}

/* Boolean */
c_ir_expr_bool_t c_ir_expr_bool_create(allocator_t allocator, bool value,
                                          location_t source_loc) {
  c_ir_expr_bool_t node = allocator_alloc(allocator, sizeof(struct _c_ir_expr_bool_t));
  node->kind = C_IR_EXPR_BOOL;
  node->source_loc = source_loc;
  node->value = value;
  return node;
}

void c_ir_expr_bool_dispose(allocator_t allocator, c_ir_expr_bool_t *node) {
  if (!node || !*node) return;
  allocator_free(allocator, node);
}
