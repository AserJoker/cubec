#include "writer/writer.h"
#include "c/c_ir.h"
#include "c/c_ir_unit.h"
#include "c/c_ir_include.h"
#include "c/c_ir_typedef.h"
#include "c/c_ir_forward_decl.h"
#include "c/c_ir_function.h"
#include "c/c_ir_variable.h"
#include "c/c_ir_enum.h"
#include "c/c_ir_stmt_block.h"
#include "c/c_ir_stmt_expr.h"
#include "c/c_ir_stmt_return.h"
#include "c/c_ir_stmt_if.h"
#include "c/c_ir_stmt_while.h"
#include "c/c_ir_stmt_do_while.h"
#include "c/c_ir_stmt_for.h"
#include "c/c_ir_stmt_jump.h"
#include "c/c_ir_stmt_local_decl.h"
#include "c/c_ir_stmt_stmt_expr.h"
#include "c/c_ir_expr_binary.h"
#include "c/c_ir_expr_unary.h"
#include "c/c_ir_expr_call.h"
#include "c/c_ir_expr_member.h"
#include "c/c_ir_expr_subscript.h"
#include "c/c_ir_expr_cast.h"
#include "c/c_ir_expr_ternary.h"
#include "c/c_ir_expr_compound.h"
#include "c/c_ir_expr_sizeof.h"
#include "c/c_ir_expr_alignof.h"
#include "c/c_ir_expr_literal.h"
#include "c/c_ir_expr_initializer.h"
#include "c/c_type.h"
#include <stdio.h>
#include <string.h>

/* ===== #line directive tracking ===== */

typedef struct _writer_state_t {
  const char *current_file;
  int current_line;
  int indent_level;
} writer_state_t;

static void emit_line_directive(string_t out, location_t loc,
                                 writer_state_t *state) {
  if (!loc.filename) return;
  if (state->current_file == NULL ||
      strcmp(loc.filename, state->current_file) != 0 ||
      (int)loc.begin.line != state->current_line) {
    char buf[256];
    snprintf(buf, sizeof(buf), "\n#line %zu \"%s\"\n",
             loc.begin.line, loc.filename);
    string_concat(out, buf);
    state->current_file = loc.filename;
    state->current_line = (int)loc.begin.line;
  }
}

static void emit_indent(string_t out, writer_state_t *state) {
  for (int i = 0; i < state->indent_level; i++) {
    string_concat(out, "    ");
  }
}

/* ===== Type writing ===== */

static void write_type(string_t out, c_type_t type, const char *name) {
  string_concat(out, string_get(type->left));
  if (name) {
    string_concat(out, " ");
    string_concat(out, name);
  }
  if (string_get_length(type->right) > 0) {
    string_concat(out, string_get(type->right));
  }
}

static void write_type_only(string_t out, c_type_t type) {
  write_type(out, type, NULL);
}

/* ===== Expression writing ===== */

static void write_expr(string_t out, c_ir_node_t node, writer_state_t *state);

static void write_expr_list(string_t out, vec_t exprs, writer_state_t *state) {
  size_t count = exprs ? vec_get_size(exprs) : 0;
  for (size_t i = 0; i < count; i++) {
    c_ir_node_t expr = vec_get(exprs, i);
    if (i > 0) string_concat(out, ", ");
    write_expr(out, expr, state);
  }
}

static void write_expr(string_t out, c_ir_node_t node, writer_state_t *state) {
  if (!node) { string_concat(out, "/* NULL */"); return; }

  switch (c_ir_get_kind(node)) {
  case C_IR_EXPR_BINARY: {
    c_ir_expr_binary_t n = (c_ir_expr_binary_t)node;
    string_concat(out, "(");
    write_expr(out, n->left, state);
    string_concat(out, " ");
    string_concat(out, string_get(n->op));
    string_concat(out, " ");
    write_expr(out, n->right, state);
    string_concat(out, ")");
    break;
  }
  case C_IR_EXPR_UNARY: {
    c_ir_expr_unary_t n = (c_ir_expr_unary_t)node;
    if (n->is_prefix) {
      string_concat(out, string_get(n->op));
      string_concat(out, "(");
      write_expr(out, n->operand, state);
      string_concat(out, ")");
    } else {
      string_concat(out, "(");
      write_expr(out, n->operand, state);
      string_concat(out, ")");
      string_concat(out, string_get(n->op));
    }
    break;
  }
  case C_IR_EXPR_CALL: {
    c_ir_expr_call_t n = (c_ir_expr_call_t)node;
    write_expr(out, n->callee, state);
    string_concat(out, "(");
    write_expr_list(out, n->arguments, state);
    string_concat(out, ")");
    break;
  }
  case C_IR_EXPR_MEMBER: {
    c_ir_expr_member_t n = (c_ir_expr_member_t)node;
    write_expr(out, n->object, state);
    string_concat(out, n->is_arrow ? "->" : ".");
    string_concat(out, string_get(n->field));
    break;
  }
  case C_IR_EXPR_SUBSCRIPT: {
    c_ir_expr_subscript_t n = (c_ir_expr_subscript_t)node;
    write_expr(out, n->object, state);
    string_concat(out, "[");
    write_expr(out, n->index, state);
    string_concat(out, "]");
    break;
  }
  case C_IR_EXPR_CAST: {
    c_ir_expr_cast_t n = (c_ir_expr_cast_t)node;
    string_concat(out, "(");
    write_type_only(out, n->type);
    string_concat(out, ")");
    string_concat(out, "(");
    write_expr(out, n->operand, state);
    string_concat(out, ")");
    break;
  }
  case C_IR_EXPR_TERNARY: {
    c_ir_expr_ternary_t n = (c_ir_expr_ternary_t)node;
    string_concat(out, "(");
    write_expr(out, n->condition, state);
    string_concat(out, " ? ");
    write_expr(out, n->consequent, state);
    string_concat(out, " : ");
    write_expr(out, n->alternate, state);
    string_concat(out, ")");
    break;
  }
  case C_IR_EXPR_COMPOUND: {
    c_ir_expr_compound_t n = (c_ir_expr_compound_t)node;
    string_concat(out, "(");
    write_type_only(out, n->type);
    string_concat(out, "){");
    write_expr_list(out, n->fields, state);
    string_concat(out, "}");
    break;
  }
  case C_IR_EXPR_SIZEOF: {
    c_ir_expr_sizeof_t n = (c_ir_expr_sizeof_t)node;
    string_concat(out, "sizeof(");
    write_type_only(out, n->type);
    string_concat(out, ")");
    break;
  }
  case C_IR_EXPR_ALIGNOF: {
    c_ir_expr_alignof_t n = (c_ir_expr_alignof_t)node;
    string_concat(out, "_Alignof(");
    write_type_only(out, n->type);
    string_concat(out, ")");
    break;
  }
  case C_IR_EXPR_STRING: {
    c_ir_expr_string_t n = (c_ir_expr_string_t)node;
    string_concat(out, string_get(n->value));
    break;
  }
  case C_IR_EXPR_NUMERIC: {
    c_ir_expr_numeric_t n = (c_ir_expr_numeric_t)node;
    string_concat(out, string_get(n->value));
    break;
  }
  case C_IR_EXPR_CHAR: {
    c_ir_expr_char_t n = (c_ir_expr_char_t)node;
    string_concat(out, string_get(n->value));
    break;
  }
  case C_IR_EXPR_IDENT: {
    c_ir_expr_ident_t n = (c_ir_expr_ident_t)node;
    string_concat(out, string_get(n->name));
    break;
  }
  case C_IR_EXPR_NULL: {
    string_concat(out, "NULL");
    break;
  }
  case C_IR_EXPR_BOOL: {
    c_ir_expr_bool_t n = (c_ir_expr_bool_t)node;
    string_concat(out, n->value ? "true" : "false");
    break;
  }
  case C_IR_EXPR_INITIALIZER: {
    c_ir_expr_initializer_t n = (c_ir_expr_initializer_t)node;
    if (n->is_designated && n->name) {
      string_concat(out, ".");
      string_concat(out, string_get(n->name));
      string_concat(out, " = ");
    }
    write_expr(out, n->value, state);
    break;
  }
  default:
    string_concat(out, "/* unknown expr */");
    break;
  }
}

/* ===== Statement writing ===== */

static void write_stmt(string_t out, c_ir_node_t node, writer_state_t *state);

static void write_stmt_list(string_t out, vec_t stmts, writer_state_t *state) {
  size_t count = stmts ? vec_get_size(stmts) : 0;
  for (size_t i = 0; i < count; i++) {
    c_ir_node_t s = vec_get(stmts, i);
    write_stmt(out, s, state);
  }
}

static void write_stmt(string_t out, c_ir_node_t node, writer_state_t *state) {
  if (!node) return;

  emit_line_directive(out, c_ir_get_source_loc(node), state);

  switch (c_ir_get_kind(node)) {
  case C_IR_STMT_BLOCK: {
    c_ir_stmt_block_t n = (c_ir_stmt_block_t)node;
    emit_indent(out, state);
    string_concat(out, "{\n");
    state->indent_level++;
    write_stmt_list(out, n->statements, state);
    state->indent_level--;
    emit_indent(out, state);
    string_concat(out, "}\n");
    break;
  }
  case C_IR_STMT_EXPR: {
    c_ir_stmt_expr_t n = (c_ir_stmt_expr_t)node;
    emit_indent(out, state);
    write_expr(out, n->expression, state);
    string_concat(out, ";\n");
    break;
  }
  case C_IR_STMT_RETURN: {
    c_ir_stmt_return_t n = (c_ir_stmt_return_t)node;
    emit_indent(out, state);
    string_concat(out, "return");
    if (n->value) {
      string_concat(out, " ");
      write_expr(out, n->value, state);
    }
    string_concat(out, ";\n");
    break;
  }
  case C_IR_STMT_IF: {
    c_ir_stmt_if_t n = (c_ir_stmt_if_t)node;
    emit_indent(out, state);
    string_concat(out, "if (");
    write_expr(out, n->condition, state);
    string_concat(out, ") ");
    /* then branch — inline block without extra indent */
    if (n->then_branch && c_ir_get_kind(n->then_branch) == C_IR_STMT_BLOCK) {
      c_ir_stmt_block_t block = (c_ir_stmt_block_t)n->then_branch;
      string_concat(out, "{\n");
      state->indent_level++;
      write_stmt_list(out, block->statements, state);
      state->indent_level--;
      emit_indent(out, state);
      string_concat(out, "}");
    } else {
      string_concat(out, "{\n");
      state->indent_level++;
      write_stmt(out, n->then_branch, state);
      state->indent_level--;
      emit_indent(out, state);
      string_concat(out, "}");
    }
    if (n->else_branch) {
      string_concat(out, " else ");
      if (c_ir_get_kind(n->else_branch) == C_IR_STMT_BLOCK) {
        c_ir_stmt_block_t block = (c_ir_stmt_block_t)n->else_branch;
        string_concat(out, "{\n");
        state->indent_level++;
        write_stmt_list(out, block->statements, state);
        state->indent_level--;
        emit_indent(out, state);
        string_concat(out, "}");
      } else if (c_ir_get_kind(n->else_branch) == C_IR_STMT_IF) {
        write_stmt(out, n->else_branch, state);
        break; /* else-if already emitted newline */
      } else {
        string_concat(out, "{\n");
        state->indent_level++;
        write_stmt(out, n->else_branch, state);
        state->indent_level--;
        emit_indent(out, state);
        string_concat(out, "}");
      }
    }
    string_concat(out, "\n");
    break;
  }
  case C_IR_STMT_WHILE: {
    c_ir_stmt_while_t n = (c_ir_stmt_while_t)node;
    emit_indent(out, state);
    string_concat(out, "while (");
    write_expr(out, n->condition, state);
    string_concat(out, ") {\n");
    state->indent_level++;
    write_stmt(out, n->body, state);
    state->indent_level--;
    emit_indent(out, state);
    string_concat(out, "}\n");
    break;
  }
  case C_IR_STMT_DO_WHILE: {
    c_ir_stmt_do_while_t n = (c_ir_stmt_do_while_t)node;
    emit_indent(out, state);
    string_concat(out, "do {\n");
    state->indent_level++;
    write_stmt(out, n->body, state);
    state->indent_level--;
    emit_indent(out, state);
    string_concat(out, "} while (");
    write_expr(out, n->condition, state);
    string_concat(out, ");\n");
    break;
  }
  case C_IR_STMT_FOR: {
    c_ir_stmt_for_t n = (c_ir_stmt_for_t)node;
    emit_indent(out, state);
    string_concat(out, "for (");
    if (n->init) write_expr(out, n->init, state);
    string_concat(out, "; ");
    if (n->condition) write_expr(out, n->condition, state);
    string_concat(out, "; ");
    if (n->update) write_expr(out, n->update, state);
    string_concat(out, ") {\n");
    state->indent_level++;
    write_stmt(out, n->body, state);
    state->indent_level--;
    emit_indent(out, state);
    string_concat(out, "}\n");
    break;
  }
  case C_IR_STMT_BREAK: {
    emit_indent(out, state);
    string_concat(out, "break;\n");
    break;
  }
  case C_IR_STMT_CONTINUE: {
    emit_indent(out, state);
    string_concat(out, "continue;\n");
    break;
  }
  case C_IR_STMT_GOTO: {
    c_ir_stmt_goto_t n = (c_ir_stmt_goto_t)node;
    emit_indent(out, state);
    string_concat(out, "goto ");
    string_concat(out, string_get(n->label));
    string_concat(out, ";\n");
    break;
  }
  case C_IR_STMT_LABEL: {
    c_ir_stmt_label_t n = (c_ir_stmt_label_t)node;
    emit_indent(out, state);
    string_concat(out, string_get(n->label));
    string_concat(out, ":\n");
    break;
  }
  case C_IR_STMT_LOCAL_DECL: {
    c_ir_stmt_local_decl_t n = (c_ir_stmt_local_decl_t)node;
    emit_indent(out, state);
    write_type(out, n->type, string_get(n->name));
    if (n->init) {
      string_concat(out, " = ");
      write_expr(out, n->init, state);
    }
    string_concat(out, ";\n");
    break;
  }
  case C_IR_STMT_STMT_EXPR: {
    c_ir_stmt_stmt_expr_t n = (c_ir_stmt_stmt_expr_t)node;
    emit_indent(out, state);
    string_concat(out, "({\n");
    state->indent_level++;
    write_stmt_list(out, n->statements, state);
    state->indent_level--;
    emit_indent(out, state);
    string_concat(out, "})\n");
    break;
  }
  default:
    break;
  }
}

/* ===== Declaration writing ===== */

static void write_function_params(string_t out, vec_t params) {
  string_concat(out, "(");
  size_t count = params ? vec_get_size(params) : 0;
  for (size_t i = 0; i < count; i++) {
    c_ir_param_t param = vec_get(params, i);
    if (i > 0) string_concat(out, ", ");
    write_type(out, param->type, string_get(param->name));
  }
  if (count == 0) string_concat(out, "void");
  string_concat(out, ")");
}

static void write_function_attributes(string_t out, c_ir_function_def_t fn) {
  if (fn->is_hidden) {
    string_concat(out, "__attribute__((no_instrument_function, noinline))\n");
  } else if (fn->is_artificial) {
    string_concat(out, "__attribute__((artificial))\n");
  }
}

static void write_function_decl(string_t out, c_ir_function_decl_t decl,
                                  writer_state_t *state) {
  if (decl->is_static) string_concat(out, "static ");
  if (decl->is_inline) string_concat(out, "inline ");
  write_type(out, decl->return_type, string_get(decl->name));
  write_function_params(out, decl->params);
  string_concat(out, ";\n");
}

static void write_function_def(string_t out, c_ir_function_def_t def,
                                 writer_state_t *state) {
  string_concat(out, "\n");
  emit_line_directive(out, def->source_loc, state);
  write_function_attributes(out, def);
  if (def->is_static) string_concat(out, "static ");
  if (def->is_inline) string_concat(out, "inline ");
  write_type(out, def->return_type, string_get(def->name));
  write_function_params(out, def->params);
  string_concat(out, " ");
  write_stmt(out, def->body, state);
  string_concat(out, "\n");
}

/* ===== Top-level writing ===== */

static const location_t NULL_LOC = {0};

void writer_write_unit(allocator_t allocator, c_ir_unit_t unit,
                         string_t out_h, string_t out_c) {
  writer_state_t state = {.current_file = NULL, .current_line = 0, .indent_level = 0};
  (void)allocator;

  /* ===== .h file ===== */
  string_t guard = allocator_create(allocator, &g_string_type,
                                      &(string_init_t){.str = "_H_CUBEC_GENERATED_"});
  string_concat(guard, string_get(unit->module_hash));
  string_concat(guard, "_H_");

  /* Uppercase the guard */
  const char *g = string_get(guard);
  char *upper = allocator_alloc(allocator, strlen(g) + 1);
  for (size_t i = 0; g[i]; i++) {
    upper[i] = (g[i] >= 'a' && g[i] <= 'z') ? g[i] - 32 : g[i];
  }
  upper[strlen(g)] = '\0';

  string_concat(out_h, "#ifndef ");
  string_concat(out_h, upper);
  string_concat(out_h, "\n#define ");
  string_concat(out_h, upper);
  string_concat(out_h, "\n\n");

  /* System includes */
  size_t inc_count = vec_get_size(unit->includes);
  for (size_t i = 0; i < inc_count; i++) {
    c_ir_include_t inc = vec_get(unit->includes, i);
    string_concat(out_h, "#include ");
    if (inc->is_system) {
      string_concat(out_h, "<");
      string_concat(out_h, string_get(inc->path));
      string_concat(out_h, ">");
    } else {
      string_concat(out_h, "\"");
      string_concat(out_h, string_get(inc->path));
      string_concat(out_h, "\"");
    }
    string_concat(out_h, "\n");
  }
  string_concat(out_h, "\n");

  /* Forward declarations */
  size_t fwd_count = vec_get_size(unit->forward_decls);
  for (size_t i = 0; i < fwd_count; i++) {
    c_ir_forward_decl_t fwd = vec_get(unit->forward_decls, i);
    string_concat(out_h, "typedef struct ");
    string_concat(out_h, string_get(fwd->name));
    string_concat(out_h, " ");
    string_concat(out_h, string_get(fwd->name));
    string_concat(out_h, ";\n");
  }
  if (fwd_count > 0) string_concat(out_h, "\n");

  /* Typedefs */
  size_t td_count = vec_get_size(unit->typedefs);
  for (size_t i = 0; i < td_count; i++) {
    c_ir_typedef_t td = vec_get(unit->typedefs, i);
    string_concat(out_h, "typedef ");
    write_type_only(out_h, td->type);
    string_concat(out_h, " ");
    string_concat(out_h, string_get(td->name));
    string_concat(out_h, ";\n");
  }
  if (td_count > 0) string_concat(out_h, "\n");

  /* Enum definitions */
  size_t enum_count = vec_get_size(unit->enum_defs);
  for (size_t i = 0; i < enum_count; i++) {
    c_ir_enum_def_t ed = vec_get(unit->enum_defs, i);
    string_concat(out_h, "typedef ");
    if (ed->backing_type) {
      string_concat(out_h, string_get(ed->backing_type->left));
      string_concat(out_h, ":");
    }
    string_concat(out_h, "enum {\n");
    size_t item_count = vec_get_size(ed->items);
    for (size_t j = 0; j < item_count; j++) {
      c_ir_enum_item_t item = vec_get(ed->items, j);
      string_concat(out_h, "    ");
      string_concat(out_h, string_get(item->name));
      string_concat(out_h, " = ");
      string_concat(out_h, string_get(item->value));
      if (j + 1 < item_count) string_concat(out_h, ",");
      string_concat(out_h, "\n");
    }
    string_concat(out_h, "} ");
    string_concat(out_h, string_get(ed->name));
    string_concat(out_h, ";\n\n");
  }

  /* Function declarations (exported) */
  size_t fn_decl_count = vec_get_size(unit->function_decls);
  for (size_t i = 0; i < fn_decl_count; i++) {
    c_ir_function_decl_t decl = vec_get(unit->function_decls, i);
    write_function_decl(out_h, decl, &state);
  }

  string_concat(out_h, "\n#endif /* ");
  string_concat(out_h, upper);
  string_concat(out_h, " */\n");

  /* ===== .c file ===== */
  string_concat(out_c, "#include \"");
  string_concat(out_c, string_get(unit->filename));
  string_concat(out_c, ".h\"\n\n");

  /* Variable declarations */
  size_t var_count = vec_get_size(unit->variable_decls);
  for (size_t i = 0; i < var_count; i++) {
    c_ir_variable_decl_t vd = vec_get(unit->variable_decls, i);
    if (vd->is_static) string_concat(out_c, "static ");
    if (vd->is_extern) string_concat(out_c, "extern ");
    write_type(out_c, vd->type, string_get(vd->name));
    if (vd->init) {
      string_concat(out_c, " = ");
      write_expr(out_c, vd->init, &state);
    }
    string_concat(out_c, ";\n");
  }
  if (var_count > 0) string_concat(out_c, "\n");

  /* Function definitions */
  size_t fn_def_count = vec_get_size(unit->function_defs);
  for (size_t i = 0; i < fn_def_count; i++) {
    c_ir_function_def_t def = vec_get(unit->function_defs, i);
    write_function_def(out_c, def, &state);
  }

  allocator_free(allocator, &upper);
  allocator_free(allocator, &guard);
}
