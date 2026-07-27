#ifndef _H_CUBEC_C_IR_EXPR_LITERAL_
#define _H_CUBEC_C_IR_EXPR_LITERAL_
#include "c/c_ir.h"
#ifdef __cplusplus
extern "C" {
#endif

/* String literal */
typedef struct _c_ir_expr_string_t *c_ir_expr_string_t;

struct _c_ir_expr_string_t {
  enum c_ir_kind kind;
  location_t source_loc;
  string_t value;     /**< Including quotes and escapes */
};

c_ir_expr_string_t c_ir_expr_string_create(allocator_t allocator, const char *value,
                                              location_t source_loc);
void c_ir_expr_string_dispose(allocator_t allocator, c_ir_expr_string_t *node);

/* Numeric literal */
typedef struct _c_ir_expr_numeric_t *c_ir_expr_numeric_t;

struct _c_ir_expr_numeric_t {
  enum c_ir_kind kind;
  location_t source_loc;
  string_t value;     /**< "42", "3.14", "0xFF", etc. */
};

c_ir_expr_numeric_t c_ir_expr_numeric_create(allocator_t allocator, const char *value,
                                                location_t source_loc);
void c_ir_expr_numeric_dispose(allocator_t allocator, c_ir_expr_numeric_t *node);

/* Char literal */
typedef struct _c_ir_expr_char_t *c_ir_expr_char_t;

struct _c_ir_expr_char_t {
  enum c_ir_kind kind;
  location_t source_loc;
  string_t value;     /**< Including quotes: "'a'" */
};

c_ir_expr_char_t c_ir_expr_char_create(allocator_t allocator, const char *value,
                                          location_t source_loc);
void c_ir_expr_char_dispose(allocator_t allocator, c_ir_expr_char_t *node);

/* Identifier */
typedef struct _c_ir_expr_ident_t *c_ir_expr_ident_t;

struct _c_ir_expr_ident_t {
  enum c_ir_kind kind;
  location_t source_loc;
  string_t name;      /**< Mangled identifier */
};

c_ir_expr_ident_t c_ir_expr_ident_create(allocator_t allocator, const char *name,
                                            location_t source_loc);
void c_ir_expr_ident_dispose(allocator_t allocator, c_ir_expr_ident_t *node);

/* NULL */
typedef struct _c_ir_expr_null_t *c_ir_expr_null_t;

struct _c_ir_expr_null_t {
  enum c_ir_kind kind;
  location_t source_loc;
};

c_ir_expr_null_t c_ir_expr_null_create(allocator_t allocator, location_t source_loc);
void c_ir_expr_null_dispose(allocator_t allocator, c_ir_expr_null_t *node);

/* Boolean */
typedef struct _c_ir_expr_bool_t *c_ir_expr_bool_t;

struct _c_ir_expr_bool_t {
  enum c_ir_kind kind;
  location_t source_loc;
  bool value;
};

c_ir_expr_bool_t c_ir_expr_bool_create(allocator_t allocator, bool value,
                                          location_t source_loc);
void c_ir_expr_bool_dispose(allocator_t allocator, c_ir_expr_bool_t *node);

#ifdef __cplusplus
}
#endif
#endif
