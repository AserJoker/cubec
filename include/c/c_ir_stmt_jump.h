#ifndef _H_CUBEC_C_IR_STMT_JUMP_
#define _H_CUBEC_C_IR_STMT_JUMP_
#include "c/c_ir.h"
#ifdef __cplusplus
extern "C" {
#endif

/* break */
typedef struct _c_ir_stmt_break_t *c_ir_stmt_break_t;

struct _c_ir_stmt_break_t {
  enum c_ir_kind kind;
  location_t source_loc;
};

c_ir_stmt_break_t c_ir_stmt_break_create(allocator_t allocator, location_t source_loc);
void c_ir_stmt_break_dispose(allocator_t allocator, c_ir_stmt_break_t *node);

/* continue */
typedef struct _c_ir_stmt_continue_t *c_ir_stmt_continue_t;

struct _c_ir_stmt_continue_t {
  enum c_ir_kind kind;
  location_t source_loc;
};

c_ir_stmt_continue_t c_ir_stmt_continue_create(allocator_t allocator, location_t source_loc);
void c_ir_stmt_continue_dispose(allocator_t allocator, c_ir_stmt_continue_t *node);

/* goto */
typedef struct _c_ir_stmt_goto_t *c_ir_stmt_goto_t;

struct _c_ir_stmt_goto_t {
  enum c_ir_kind kind;
  location_t source_loc;
  string_t label;
};

c_ir_stmt_goto_t c_ir_stmt_goto_create(allocator_t allocator, const char *label,
                                         location_t source_loc);
void c_ir_stmt_goto_dispose(allocator_t allocator, c_ir_stmt_goto_t *node);

/* label */
typedef struct _c_ir_stmt_label_t *c_ir_stmt_label_t;

struct _c_ir_stmt_label_t {
  enum c_ir_kind kind;
  location_t source_loc;
  string_t label;
};

c_ir_stmt_label_t c_ir_stmt_label_create(allocator_t allocator, const char *label,
                                           location_t source_loc);
void c_ir_stmt_label_dispose(allocator_t allocator, c_ir_stmt_label_t *node);

#ifdef __cplusplus
}
#endif
#endif
