/**
 * @file cubec_ast_writer.c
 * @brief Cubec AST pretty printer — serializes the entire AST back to
 *        valid Cubec source code for round-trip testing and debugging.
 *
 * Usage:
 *   string_t out = type_new(allocator, g_string_type, &(string_init_t){NULL});
 *   cubec_ast_write(allocator, program_node, out);
 *   const char *source = string_get(out);
 *
 * Architecture:
 *   The writer recursively dispatches on node->kind through a series of
 *   switch statements split by node category (expression, statement,
 *   declaration, type). Each handler emits the corresponding Cubec syntax.
 */

#include "writer/cubec_ast_writer.h"

/* --- Core infrastructure --- */
#include "core/allocator.h"
#include "core/node.h"
#include "core/string.h"
#include "core/type.h"
#include "core/vec.h"

/* --- Literal nodes --- */
#include "cubec/literal.h"
#include "cubec/literal_identifier.h"
#include "cubec/literal_numeric.h"
#include "cubec/literal_string.h"
#include "cubec/literal_char.h"

/* --- Expression nodes --- */
#include "cubec/expression.h"
#include "cubec/expression_binary.h"
#include "cubec/expression_call.h"
#include "cubec/expression_member.h"
#include "cubec/expression_subscript.h"
#include "cubec/expression_group.h"
#include "cubec/expression_ternary.h"
#include "cubec/expression_comma.h"
#include "cubec/expression_function.h"
#include "cubec/expression_assignment.h"
#include "cubec/expression_generic_instantiation.h"
#include "cubec/expression_namespace_access.h"
#include "cubec/expression_slice.h"
#include "cubec/expression_sizeof.h"
#include "cubec/expression_typeof.h"
#include "cubec/expression_alignof.h"
#include "cubec/expression_initialize_list.h"
#include "cubec/expression_initialize_field.h"
#include "cubec/expression_spread.h"
#include "cubec/expression_wildcard.h"
#include "cubec/expression_type_qualifier.h"
#include "cubec/expression_type_tuple.h"

/* --- Declaration nodes --- */
#include "cubec/declaration.h"
#include "cubec/declaration_variable.h"
#include "cubec/declaration_pointer.h"
#include "cubec/declaration_array.h"
#include "cubec/declaration_slice.h"

/* --- Statement nodes --- */
#include "cubec/statement_function.h"
#include "cubec/statement_block.h"
#include "cubec/statement_if.h"
#include "cubec/statement_while.h"
#include "cubec/statement_for.h"
#include "cubec/statement_foreach.h"
#include "cubec/statement_do_while.h"
#include "cubec/statement_return.h"
#include "cubec/statement_switch.h"
#include "cubec/statement_expression.h"
#include "cubec/statement_declaration.h"
#include "cubec/statement_struct.h"
#include "cubec/statement_enum.h"
#include "cubec/statement_union.h"
#include "cubec/statement_cunion.h"
#include "cubec/statement_declaration_type.h"
#include "cubec/statement_defer.h"
#include "cubec/statement_test.h"
#include "cubec/statement_import.h"
#include "cubec/statement_interface.h"
#include "cubec/statement_break.h"
#include "cubec/statement_continue.h"
#include "cubec/statement_empty.h"

/* --- Auxiliary nodes --- */
#include "cubec/function_argument.h"
#include "cubec/function_capture.h"
#include "cubec/generic_param.h"
#include "cubec/struct_field.h"
#include "cubec/union_field.h"
#include "cubec/enum_item.h"
#include "cubec/switch_match.h"
#include "cubec/decorator.h"

/* --- Program node --- */
#include "cubec/program.h"

/* ====================================================================
 *  Forward declarations
 * ==================================================================== */

static void _write_expr(allocator_t a, string_t out, node_t n);
static void _write_type_expr(allocator_t a, string_t out, node_t n);
static void _write_stmt(allocator_t a, string_t out, node_t n, int indent);


/* ====================================================================
 *  Helpers
 * ==================================================================== */

static const char *_id_name(node_t id) {
  return string_get(((cubec_literal_identifier_t)id)->value);
}

static void _indent(string_t out, int level) {
  for (int i = 0; i < level; i++) string_concat(out, "  ");
}

static void _write_args(allocator_t a, string_t out, vec_t args) {
  if (!args) return;
  for (size_t i = 0; i < vec_get_size(args); i++) {
    if (i > 0) string_concat(out, ", ");
    cubec_function_argument_t arg = (cubec_function_argument_t)vec_get(args, i);
    if (arg->is_rest) string_concat(out, "...");
    string_concat(out, _id_name(arg->identifier));
    if (arg->type) {
      string_concat(out, ": ");
      _write_type_expr(a, out, arg->type);
    }
  }
}

static void _write_generic_params(allocator_t a, string_t out, vec_t params) {
  if (!params || vec_get_size(params) == 0) return;
  string_concat(out, "[");
  for (size_t i = 0; i < vec_get_size(params); i++) {
    if (i > 0) string_concat(out, ", ");
    cubec_generic_param_t gp = (cubec_generic_param_t)vec_get(params, i);
    if (gp->is_rest) string_concat(out, "...");
    string_concat(out, _id_name(gp->name));
    if (gp->value_type) {
      string_concat(out, ": ");
      _write_type_expr(a, out, gp->value_type);
    }
    if (gp->constraints && vec_get_size(gp->constraints) > 0) {
      string_concat(out, " extends ");
      for (size_t j = 0; j < vec_get_size(gp->constraints); j++) {
        if (j > 0) string_concat(out, " & ");
        _write_type_expr(a, out, (node_t)vec_get(gp->constraints, j));
      }
    }
  }
  string_concat(out, "]");
}

static void _write_str_literal(string_t out, const char *s) {
  string_concat(out, "\"");
  if (s) string_concat(out, s);
  string_concat(out, "\"");
}

static void _write_escaped_char(string_t out, char c) {
  switch (c) {
  case '\n': string_concat(out, "'\\n'"); break;
  case '\r': string_concat(out, "'\\r'"); break;
  case '\t': string_concat(out, "'\\t'"); break;
  case '\0': string_concat(out, "'\\0'"); break;
  case '\\': string_concat(out, "'\\\\'"); break;
  case '\'': string_concat(out, "'\\''"); break;
  default: {
    char buf[8];
    int n = snprintf(buf, sizeof(buf), "'%c'", c);
    if (n > 0 && n < (int)sizeof(buf)) string_nconcat(out, buf, (size_t)n);
    break;
  }
  }
}

/* ====================================================================
 *  Type expression writers
 * ==================================================================== */

static void _write_type_expr(allocator_t a, string_t out, node_t n) {
  if (!n) { string_concat(out, "void"); return; }

  switch (n->kind) {
  case CUBEC_NODE_LITERAL_IDENTIFIER:
    string_concat(out, _id_name(n));
    break;

  case CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS: {
    cubec_expression_namespace_access_t ns =
        (cubec_expression_namespace_access_t)n;
    _write_type_expr(a, out, ns->host);
    string_concat(out, "::");
    string_concat(out, _id_name((node_t)ns->field));
    break;
  }

  case CUBEC_NODE_DECLARATION_POINTER: {
    cubec_declaration_pointer_t p = (cubec_declaration_pointer_t)n;
    _write_type_expr(a, out, p->type);
    string_concat(out, "*");
    break;
  }

  case CUBEC_NODE_DECLARATION_SLICE: {
    cubec_declaration_slice_t s = (cubec_declaration_slice_t)n;
    if (s->is_const) string_concat(out, "const ");
    if (s->is_volatile) string_concat(out, "volatile ");
    string_concat(out, "[]");
    _write_type_expr(a, out, s->type);
    break;
  }

  case CUBEC_NODE_DECLARATION_ARRAY: {
    cubec_declaration_array_t arr = (cubec_declaration_array_t)n;
    string_concat(out, "[");
    if (arr->size) _write_expr(a, out, arr->size);
    string_concat(out, "]");
    _write_type_expr(a, out, arr->type);
    break;
  }

  case CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION: {
    cubec_expression_generic_instantiation_t gi =
        (cubec_expression_generic_instantiation_t)n;
    _write_type_expr(a, out, gi->callee);
    string_concat(out, "[");
    if (gi->arguments) {
      for (size_t i = 0; i < vec_get_size(gi->arguments); i++) {
        if (i > 0) string_concat(out, ", ");
        _write_type_expr(a, out, (node_t)vec_get(gi->arguments, i));
      }
    }
    string_concat(out, "]");
    break;
  }

  case CUBEC_NODE_EXPRESSION_TYPE_QUALIFIER: {
    cubec_expression_type_qualifier_t tq =
        (cubec_expression_type_qualifier_t)n;
    if (tq->is_const) string_concat(out, "const ");
    if (tq->is_volatile) string_concat(out, "volatile ");
    _write_type_expr(a, out, tq->type);
    break;
  }

  case CUBEC_NODE_EXPRESSION_TYPE_TUPLE: {
    cubec_expression_type_tuple_t tt =
        (cubec_expression_type_tuple_t)n;
    string_concat(out, "<");
    if (tt->element_types) {
      for (size_t i = 0; i < vec_get_size(tt->element_types); i++) {
        if (i > 0) string_concat(out, ", ");
        _write_type_expr(a, out, (node_t)vec_get(tt->element_types, i));
      }
    }
    string_concat(out, ">");
    break;
  }

  /* Type expression nodes that use declaration_variable as container */
  case CUBEC_NODE_DECLARATION_VARIABLE: {
    /* In type context, this is the TUPLE degrade result: struct tuples */
    cubec_declaration_variable_t dv = (cubec_declaration_variable_t)n;
    if (dv->identifier) {
      string_concat(out, _id_name(dv->identifier));
    } else {
      string_concat(out, "_");
    }
    break;
  }

  default:
    /* Fallback: try as expression (will render, maybe as identifier) */
    _write_expr(a, out, n);
    break;
  }
}

/* ====================================================================
 *  Expression writers
 * ==================================================================== */

static void _write_expr(allocator_t a, string_t out, node_t n) {
  if (!n) return;

  switch (n->kind) {
  /* --- Literals --- */
  case CUBEC_NODE_LITERAL_IDENTIFIER:
    string_concat(out, _id_name(n));
    break;

  case CUBEC_NODE_LITERAL_NUMERIC: {
    cubec_literal_numeric_t num = (cubec_literal_numeric_t)n;
    string_concat(out, string_get(num->value));
    break;
  }

  case CUBEC_NODE_LITERAL_STRING: {
    cubec_literal_string_t s = (cubec_literal_string_t)n;
    _write_str_literal(out, string_get(s->value));
    break;
  }

  case CUBEC_NODE_LITERAL_CHAR: {
    cubec_literal_char_t ch = (cubec_literal_char_t)n;
    _write_escaped_char(out, ch->value);
    break;
  }

  case CUBEC_NODE_LITERAL_UNDEFINED:
    string_concat(out, "undefined");
    break;

  case CUBEC_NODE_EXPRESSION_WILDCARD:
    string_concat(out, "_");
    break;

  /* --- Binary / Unary --- */
  case CUBEC_NODE_EXPRESSION_BINARY: {
    cubec_expression_binary_t b = (cubec_expression_binary_t)n;
    const char *op = string_get(b->opt);
    if (b->left) {
      _write_expr(a, out, b->left);
      string_concat(out, " ");
      string_concat(out, op);
      string_concat(out, " ");
      _write_expr(a, out, b->right);
    } else {
      /* Prefix unary */
      string_concat(out, op);
      _write_expr(a, out, b->right);
    }
    break;
  }

  /* --- Postfix unary: .* .& .? .! --- */
  case CUBEC_NODE_EXPRESSION_DEREF:
  case CUBEC_NODE_EXPRESSION_ADDR:
  case CUBEC_NODE_EXPRESSION_TRY:
  case CUBEC_NODE_EXPRESSION_ASSERT: {
    cubec_expression_binary_t b = (cubec_expression_binary_t)n;
    _write_expr(a, out, b->right);
    string_concat(out, string_get(b->opt));
    break;
  }

  /* --- Call --- */
  case CUBEC_NODE_EXPRESSION_CALL: {
    cubec_expression_call_t c = (cubec_expression_call_t)n;
    _write_expr(a, out, c->callee);
    string_concat(out, "(");
    if (c->arguments) {
      for (size_t i = 0; i < vec_get_size(c->arguments); i++) {
        if (i > 0) string_concat(out, ", ");
        _write_expr(a, out, (node_t)vec_get(c->arguments, i));
      }
    }
    string_concat(out, ")");
    break;
  }

  /* --- Member access --- */
  case CUBEC_NODE_EXPRESSION_MEMBER: {
    cubec_expression_member_t m = (cubec_expression_member_t)n;
    _write_expr(a, out, m->host);
    string_concat(out, ".");
    string_concat(out, _id_name((node_t)m->field));
    break;
  }

  /* --- Subscript --- */
  case CUBEC_NODE_EXPRESSION_SUBSCRIPT: {
    cubec_expression_subscript_t s = (cubec_expression_subscript_t)n;
    _write_expr(a, out, s->host);
    string_concat(out, "[");
    _write_expr(a, out, s->index);
    string_concat(out, "]");
    break;
  }

  /* --- Group --- */
  case CUBEC_NODE_EXPRESSION_GROUP: {
    cubec_expression_group_t g = (cubec_expression_group_t)n;
    string_concat(out, "(");
    _write_expr(a, out, g->inner);
    string_concat(out, ")");
    break;
  }

  /* --- Ternary --- */
  case CUBEC_NODE_EXPRESSION_TERNARY: {
    cubec_expression_ternary_t t = (cubec_expression_ternary_t)n;
    _write_expr(a, out, t->condition);
    string_concat(out, " ? ");
    _write_expr(a, out, t->consequent);
    string_concat(out, " : ");
    _write_expr(a, out, t->alternate);
    break;
  }

  /* --- Comma --- */
  case CUBEC_NODE_EXPRESSION_COMMA: {
    cubec_expression_comma_t cm = (cubec_expression_comma_t)n;
    _write_expr(a, out, cm->left);
    string_concat(out, ", ");
    _write_expr(a, out, cm->right);
    break;
  }

  /* --- Assignment --- */
  case CUBEC_NODE_EXPRESSION_ASSIGNMENT: {
    cubec_expression_binary_t asgn = (cubec_expression_binary_t)n;
    _write_expr(a, out, asgn->left);
    string_concat(out, " = ");
    _write_expr(a, out, asgn->right);
    break;
  }

  /* --- Generic instantiation (value context, e.g. Vec[i32]) --- */
  case CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION: {
    cubec_expression_generic_instantiation_t gi =
        (cubec_expression_generic_instantiation_t)n;
    _write_expr(a, out, gi->callee);
    string_concat(out, "[");
    if (gi->arguments) {
      for (size_t i = 0; i < vec_get_size(gi->arguments); i++) {
        if (i > 0) string_concat(out, ", ");
        _write_expr(a, out, (node_t)vec_get(gi->arguments, i));
      }
    }
    string_concat(out, "]");
    break;
  }

  /* --- Namespace access --- */
  case CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS: {
    cubec_expression_namespace_access_t ns =
        (cubec_expression_namespace_access_t)n;
    _write_expr(a, out, ns->host);
    string_concat(out, "::");
    string_concat(out, _id_name((node_t)ns->field));
    break;
  }

  /* --- Slice expression --- */
  case CUBEC_NODE_EXPRESSION_SLICE: {
    cubec_expression_subscript_t sl = (cubec_expression_subscript_t)n;
    _write_expr(a, out, sl->host);
    string_concat(out, "[");
    _write_expr(a, out, sl->index);
    string_concat(out, "..]");
    break;
  }

  /* --- Function expression (anonymous / lambda) --- */
  case CUBEC_NODE_EXPRESSION_FUNCTION: {
    cubec_expression_function_t fn = (cubec_expression_function_t)n;
    string_concat(out, "func");
    if (fn->name) {
      string_concat(out, " ");
      string_concat(out, _id_name(fn->name));
    }
    if (fn->captures && vec_get_size(fn->captures) > 0) {
      string_concat(out, " |");
      for (size_t i = 0; i < vec_get_size(fn->captures); i++) {
        if (i > 0) string_concat(out, ", ");
        cubec_function_capture_t cap =
            (cubec_function_capture_t)vec_get(fn->captures, i);
        string_concat(out, _id_name(cap->identifier));
      }
      string_concat(out, "|");
    }
    _write_generic_params(a, out, fn->generic_params);
    string_concat(out, "(");
    _write_args(a, out, fn->arguments);
    string_concat(out, ")");
    if (fn->return_type) {
      string_concat(out, ": ");
      _write_type_expr(a, out, fn->return_type);
    }
    if (fn->body) {
      string_concat(out, " ");
      _write_stmt(a, out, fn->body, 0);
    } else {
      string_concat(out, ";");
    }
    break;
  }

  /* --- typeof / sizeof / alignof --- */
  case CUBEC_NODE_EXPRESSION_TYPEOF: {
    cubec_expression_typeof_t t = (cubec_expression_typeof_t)n;
    string_concat(out, "typeof(");
    if (t->expression) _write_expr(a, out, t->expression);
    string_concat(out, ")");
    break;
  }

  case CUBEC_NODE_EXPRESSION_SIZEOF: {
    cubec_expression_sizeof_t s = (cubec_expression_sizeof_t)n;
    string_concat(out, "sizeof(");
    if (s->expression) _write_expr(a, out, s->expression);
    string_concat(out, ")");
    break;
  }

  case CUBEC_NODE_EXPRESSION_ALIGNOF: {
    cubec_expression_sizeof_t ao = (cubec_expression_sizeof_t)n;
    string_concat(out, "alignof(");
    if (ao->expression) _write_expr(a, out, ao->expression);
    string_concat(out, ")");
    break;
  }

  /* --- Initialize list --- */
  case CUBEC_NODE_EXPRESSION_INITIALIZE_LIST: {
    cubec_expression_initialize_list_t il =
        (cubec_expression_initialize_list_t)n;
    if (il->type) {
      string_concat(out, ".");
      _write_type_expr(a, out, il->type);
    }
    string_concat(out, "{");
    if (il->items) {
      for (size_t i = 0; i < vec_get_size(il->items); i++) {
        if (i > 0) string_concat(out, ", ");
        node_t item = (node_t)vec_get(il->items, i);
        if (il->is_field) {
          /* initialize_field */
          cubec_expression_initialize_field_t f =
              (cubec_expression_initialize_field_t)item;
          string_concat(out, ".");
          string_concat(out, _id_name((node_t)f->field));
          string_concat(out, " = ");
          _write_expr(a, out, f->value);
        } else {
          _write_expr(a, out, item);
        }
      }
    }
    string_concat(out, "}");
    break;
  }

  /* --- Spread --- */
  case CUBEC_NODE_EXPRESSION_SPREAD: {
    cubec_expression_comma_t sp = (cubec_expression_comma_t)n;
    string_concat(out, "...");
    _write_expr(a, out, sp->right);
    break;
  }

  /* --- Type qualifier used as value expression --- */
  case CUBEC_NODE_EXPRESSION_TYPE_QUALIFIER: {
    cubec_expression_type_qualifier_t tq =
        (cubec_expression_type_qualifier_t)n;
    if (tq->is_const) string_concat(out, "const ");
    if (tq->is_volatile) string_concat(out, "volatile ");
    _write_type_expr(a, out, tq->type);
    break;
  }

  /* --- Type tuple used as value expression --- */
  case CUBEC_NODE_EXPRESSION_TYPE_TUPLE: {
    cubec_expression_type_tuple_t tt =
        (cubec_expression_type_tuple_t)n;
    string_concat(out, "<");
    if (tt->element_types) {
      for (size_t i = 0; i < vec_get_size(tt->element_types); i++) {
        if (i > 0) string_concat(out, ", ");
        _write_type_expr(a, out, (node_t)vec_get(tt->element_types, i));
      }
    }
    string_concat(out, ">");
    break;
  }

  /* --- Declaration nodes used as expressions (type context) --- */
  case CUBEC_NODE_DECLARATION_POINTER:
  case CUBEC_NODE_DECLARATION_SLICE:
  case CUBEC_NODE_DECLARATION_ARRAY:
  case CUBEC_NODE_DECLARATION_VARIABLE:
    _write_type_expr(a, out, n);
    break;

  /* --- Unsupported / unexpected in expression context --- */
  case CUBEC_NODE_EXPRESSION_COMPUTE_MEMBER: {
    string_concat(out, "/* compute_member */");
    break;
  }

  default:
    string_concat(out, "/* unknown_expr(#");
    {
      char buf[16];
      snprintf(buf, sizeof(buf), "%u", (unsigned)n->kind);
      string_concat(out, buf);
    }
    string_concat(out, ") */");
    break;
  }
}

/* ====================================================================
 *  Auxiliary member writers
 * ==================================================================== */

static void _write_struct_field(allocator_t a, string_t out, node_t field,
                                 int indent) {
  cubec_struct_field_t sf = (cubec_struct_field_t)field;
  _indent(out, indent + 1);
  if (sf->is_pub) string_concat(out, "pub ");
  string_concat(out, _id_name(sf->name));
  string_concat(out, ": ");
  _write_type_expr(a, out, sf->type);
  string_concat(out, ";\n");
}

static void _write_union_field(allocator_t a, string_t out, node_t field,
                                int indent) {
  cubec_union_field_t uf = (cubec_union_field_t)field;
  _indent(out, indent + 1);
  string_concat(out, _id_name(uf->name));
  string_concat(out, ": ");
  _write_type_expr(a, out, uf->type);
  string_concat(out, ";\n");
}

static void _write_enum_item(allocator_t a, string_t out, node_t item,
                              int indent) {
  cubec_enum_item_t ei = (cubec_enum_item_t)item;
  _indent(out, indent + 1);
  string_concat(out, _id_name(ei->name));
  if (ei->type) {
    string_concat(out, ": ");
    _write_type_expr(a, out, ei->type);
  }
  if (ei->value) {
    string_concat(out, " = ");
    _write_expr(a, out, ei->value);
  }
  string_concat(out, ";\n");
}

static void _write_switch_match(allocator_t a, string_t out, node_t m,
                                 int indent) {
  cubec_switch_match_t sm = (cubec_switch_match_t)m;
  _indent(out, indent + 1);
  if (sm->is_else) {
    string_concat(out, "else -> ");
  } else {
    string_concat(out, "case(");
    if (sm->values) {
      for (size_t i = 0; i < vec_get_size(sm->values); i++) {
        if (i > 0) string_concat(out, ", ");
        _write_expr(a, out, (node_t)vec_get(sm->values, i));
      }
    }
    string_concat(out, ") -> ");
  }
  _write_stmt(a, out, sm->body, indent + 1);
}

/* ====================================================================
 *  Full statement writer (single statement, no trailing newline handling
 *  for inline use — callers handle newlines for top-level statements)
 * ==================================================================== */

static void _write_stmt(allocator_t a, string_t out, node_t n, int indent) {
  if (!n) return;

  switch (n->kind) {
  /* --- Block --- */
  case CUBEC_NODE_STATEMENT_BLOCK: {
    cubec_statement_block_t b = (cubec_statement_block_t)n;
    string_concat(out, "{\n");
    if (b->statements) {
      for (size_t i = 0; i < vec_get_size(b->statements); i++)
        _write_stmt(a, out, (node_t)vec_get(b->statements, i), indent + 1);
    }
    _indent(out, indent);
    string_concat(out, "}\n");
    break;
  }

  /* --- Expression statement --- */
  case CUBEC_NODE_STATEMENT_EXPRESSION: {
    cubec_statement_expression_t se =
        (cubec_statement_expression_t)n;
    _indent(out, indent);
    if (se->expression) _write_expr(a, out, se->expression);
    string_concat(out, ";\n");
    break;
  }

  /* --- Variable / using declaration --- */
  case CUBEC_NODE_STATEMENT_DECLARATION: {
    cubec_statement_declaration_t sd =
        (cubec_statement_declaration_t)n;
    cubec_declaration_variable_t dv =
        (cubec_declaration_variable_t)sd->declarator;
    _indent(out, indent);
    if (sd->is_comptime) string_concat(out, "comptime ");
    if (sd->is_export) string_concat(out, "export ");
    if (sd->is_extern) string_concat(out, "extern ");
    if (sd->is_using) string_concat(out, "using ");
    string_concat(out, "var ");
    if (dv->identifier) string_concat(out, _id_name(dv->identifier));
    if (dv->type) {
      string_concat(out, ": ");
      _write_type_expr(a, out, dv->type);
    }
    if (dv->expression) {
      string_concat(out, " = ");
      _write_expr(a, out, dv->expression);
    }
    string_concat(out, ";\n");
    break;
  }

  /* --- Function --- */
  case CUBEC_NODE_STATEMENT_FUNCTION: {
    cubec_statement_function_t fn =
        (cubec_statement_function_t)n;
    _indent(out, indent);
    if (fn->is_export) string_concat(out, "export ");
    if (fn->is_extern) string_concat(out, "extern ");
    if (fn->is_comptime) string_concat(out, "comptime ");
    if (fn->is_inline) string_concat(out, "inline ");
    string_concat(out, "func ");
    string_concat(out, _id_name(fn->name));
    _write_generic_params(a, out, fn->generic_params);
    string_concat(out, "(");
    _write_args(a, out, fn->arguments);
    if (fn->is_c_variadic) string_concat(out, ", ...");
    string_concat(out, ")");
    if (fn->return_type) {
      string_concat(out, ": ");
      _write_type_expr(a, out, fn->return_type);
    }
    if (fn->body) {
      string_concat(out, " ");
      _write_stmt(a, out, fn->body, indent);
    } else {
      string_concat(out, ";\n");
    }
    break;
  }

  /* --- Struct --- */
  case CUBEC_NODE_STATEMENT_STRUCT: {
    cubec_statement_struct_t s = (cubec_statement_struct_t)n;
    _indent(out, indent);
    if (s->is_export) string_concat(out, "export ");
    string_concat(out, "struct ");
    string_concat(out, _id_name(s->name));
    _write_generic_params(a, out, s->generic_params);
    if (s->implements && vec_get_size(s->implements) > 0) {
      string_concat(out, " implement ");
      for (size_t i = 0; i < vec_get_size(s->implements); i++) {
        if (i > 0) string_concat(out, ", ");
        _write_type_expr(a, out, (node_t)vec_get(s->implements, i));
      }
    }
    if (s->members) {
      string_concat(out, " {\n");
      for (size_t i = 0; i < vec_get_size(s->members); i++) {
        node_t m = (node_t)vec_get(s->members, i);
        if (m->kind == CUBEC_NODE_STRUCT_FIELD)
          _write_struct_field(a, out, m, indent);
        else
          _write_stmt(a, out, m, indent + 1);
      }
      _indent(out, indent);
      string_concat(out, "}\n");
    } else {
      string_concat(out, ";\n");
    }
    break;
  }

  /* --- Enum --- */
  case CUBEC_NODE_STATEMENT_ENUM: {
    cubec_statement_enum_t e = (cubec_statement_enum_t)n;
    _indent(out, indent);
    if (e->is_export) string_concat(out, "export ");
    string_concat(out, "enum ");
    string_concat(out, _id_name(e->name));
    if (e->items) {
      string_concat(out, " {\n");
      for (size_t i = 0; i < vec_get_size(e->items); i++)
        _write_enum_item(a, out, (node_t)vec_get(e->items, i), indent);
      _indent(out, indent);
      string_concat(out, "}\n");
    } else {
      string_concat(out, ";\n");
    }
    break;
  }

  /* --- Union --- */
  case CUBEC_NODE_STATEMENT_UNION: {
    cubec_statement_union_t u = (cubec_statement_union_t)n;
    _indent(out, indent);
    string_concat(out, "union ");
    string_concat(out, _id_name(u->name));
    _write_generic_params(a, out, u->generic_params);
    if (u->members) {
      string_concat(out, " {\n");
      for (size_t i = 0; i < vec_get_size(u->members); i++) {
        node_t m = (node_t)vec_get(u->members, i);
        if (m->kind == CUBEC_NODE_UNION_FIELD)
          _write_union_field(a, out, m, indent);
        else
          _write_stmt(a, out, m, indent + 1);
      }
      _indent(out, indent);
      string_concat(out, "}\n");
    } else {
      string_concat(out, ";\n");
    }
    break;
  }

  /* --- CUnion --- */
  case CUBEC_NODE_STATEMENT_CUNION: {
    cubec_statement_cunion_t cu = (cubec_statement_cunion_t)n;
    _indent(out, indent);
    string_concat(out, "cunion ");
    string_concat(out, _id_name(cu->name));
    if (cu->fields) {
      string_concat(out, " {\n");
      for (size_t i = 0; i < vec_get_size(cu->fields); i++)
        _write_struct_field(a, out, (node_t)vec_get(cu->fields, i), indent);
      _indent(out, indent);
      string_concat(out, "}\n");
    } else {
      string_concat(out, ";\n");
    }
    break;
  }

  /* --- Type alias --- */
  case CUBEC_NODE_STATEMENT_DECLARATION_TYPE: {
    cubec_statement_declaration_type_t td =
        (cubec_statement_declaration_type_t)n;
    _indent(out, indent);
    if (td->is_export) string_concat(out, "export ");
    if (td->is_builtin) string_concat(out, "builtin ");
    string_concat(out, "type ");
    string_concat(out, _id_name(td->name));
    if (td->params) {
      string_concat(out, "[");
      for (size_t i = 0; i < vec_get_size(td->params); i++) {
        if (i > 0) string_concat(out, ", ");
        string_concat(out, _id_name((node_t)vec_get(td->params, i)));
      }
      string_concat(out, "]");
    }
    if (td->type_value) {
      string_concat(out, " = ");
      _write_type_expr(a, out, td->type_value);
    }
    string_concat(out, ";\n");
    break;
  }

  /* --- Interface --- */
  case CUBEC_NODE_STATEMENT_INTERFACE: {
    cubec_statement_interface_t iface =
        (cubec_statement_interface_t)n;
    _indent(out, indent);
    if (iface->is_export) string_concat(out, "export ");
    string_concat(out, "interface ");
    string_concat(out, _id_name(iface->name));
    _write_generic_params(a, out, iface->generic_params);
    if (iface->members) {
      string_concat(out, " {\n");
      for (size_t i = 0; i < vec_get_size(iface->members); i++)
        _write_stmt(a, out, (node_t)vec_get(iface->members, i), indent + 1);
      _indent(out, indent);
      string_concat(out, "}\n");
    } else {
      string_concat(out, ";\n");
    }
    break;
  }

  /* --- If --- */
  case CUBEC_NODE_STATEMENT_IF: {
    cubec_statement_if_t si = (cubec_statement_if_t)n;
    _indent(out, indent);
    string_concat(out, "if (");
    _write_expr(a, out, si->condition);
    string_concat(out, ") ");
    _write_stmt(a, out, si->then_branch, indent);
    if (si->else_branch) {
      _indent(out, indent);
      string_concat(out, "else ");
      _write_stmt(a, out, si->else_branch, indent);
    }
    break;
  }

  /* --- While --- */
  case CUBEC_NODE_STATEMENT_WHILE: {
    cubec_statement_while_t w = (cubec_statement_while_t)n;
    _indent(out, indent);
    string_concat(out, "while (");
    _write_expr(a, out, w->condition);
    string_concat(out, ") ");
    _write_stmt(a, out, w->body, indent);
    break;
  }

  /* --- Do-While --- */
  case CUBEC_NODE_STATEMENT_DO_WHILE: {
    cubec_statement_do_while_t dw = (cubec_statement_do_while_t)n;
    _indent(out, indent);
    string_concat(out, "do ");
    _write_stmt(a, out, dw->body, indent);
    _indent(out, indent);
    string_concat(out, "while (");
    _write_expr(a, out, dw->condition);
    string_concat(out, ");\n");
    break;
  }

  /* --- For --- */
  case CUBEC_NODE_STATEMENT_FOR: {
    cubec_statement_for_t sf = (cubec_statement_for_t)n;
    _indent(out, indent);
    string_concat(out, "for (");
    if (sf->init) {
      node_t init = sf->init;
      if (init->kind == CUBEC_NODE_STATEMENT_EXPRESSION) {
        _write_expr(a, out,
                    ((cubec_statement_expression_t)init)->expression);
      } else if (init->kind == CUBEC_NODE_STATEMENT_DECLARATION) {
        /* Inline var decl without semicolon */
        cubec_statement_declaration_t sd =
            (cubec_statement_declaration_t)init;
        cubec_declaration_variable_t dv =
            (cubec_declaration_variable_t)sd->declarator;
        string_concat(out, "var ");
        string_concat(out, _id_name(dv->identifier));
        if (dv->type) {
          string_concat(out, ": ");
          _write_type_expr(a, out, dv->type);
        }
        if (dv->expression) {
          string_concat(out, " = ");
          _write_expr(a, out, dv->expression);
        }
      }
    }
    string_concat(out, "; ");
    if (sf->condition) _write_expr(a, out, sf->condition);
    string_concat(out, "; ");
    if (sf->increment) _write_expr(a, out, sf->increment);
    string_concat(out, ") ");
    _write_stmt(a, out, sf->body, indent);
    break;
  }

  /* --- Foreach --- */
  case CUBEC_NODE_STATEMENT_FOREACH: {
    cubec_statement_foreach_t fe = (cubec_statement_foreach_t)n;
    _indent(out, indent);
    string_concat(out, "foreach(");
    if (fe->is_var_decl) string_concat(out, "var ");
    string_concat(out, _id_name(fe->variable));
    if (fe->var_type) {
      string_concat(out, ": ");
      _write_type_expr(a, out, fe->var_type);
    }
    string_concat(out, " of ");
    _write_expr(a, out, fe->iterator);
    string_concat(out, ") ");
    _write_stmt(a, out, fe->body, indent);
    break;
  }

  /* --- Switch --- */
  case CUBEC_NODE_STATEMENT_SWITCH: {
    cubec_statement_switch_t sw = (cubec_statement_switch_t)n;
    _indent(out, indent);
    string_concat(out, "switch(");
    _write_expr(a, out, sw->condition);
    string_concat(out, ") {\n");
    if (sw->matches) {
      for (size_t i = 0; i < vec_get_size(sw->matches); i++)
        _write_switch_match(a, out,
                            (node_t)vec_get(sw->matches, i), indent);
    }
    _indent(out, indent);
    string_concat(out, "}\n");
    break;
  }

  /* --- Return --- */
  case CUBEC_NODE_STATEMENT_RETURN: {
    cubec_statement_return_t sr = (cubec_statement_return_t)n;
    _indent(out, indent);
    string_concat(out, "return");
    if (sr->expression) {
      string_concat(out, " ");
      _write_expr(a, out, sr->expression);
    }
    string_concat(out, ";\n");
    break;
  }

  /* --- Defer --- */
  case CUBEC_NODE_STATEMENT_DEFER: {
    cubec_statement_defer_t sd = (cubec_statement_defer_t)n;
    _indent(out, indent);
    string_concat(out, "defer");
    if (sd->captures && vec_get_size(sd->captures) > 0) {
      string_concat(out, " |");
      for (size_t i = 0; i < vec_get_size(sd->captures); i++) {
        if (i > 0) string_concat(out, ", ");
        cubec_function_capture_t cap =
            (cubec_function_capture_t)vec_get(sd->captures, i);
        string_concat(out, _id_name(cap->identifier));
      }
      string_concat(out, "|");
    }
    string_concat(out, " ");
    _write_stmt(a, out, sd->body, indent);
    break;
  }

  /* --- Break / Continue / Empty --- */
  case CUBEC_NODE_STATEMENT_BREAK:
    _indent(out, indent);
    string_concat(out, "break;\n");
    break;

  case CUBEC_NODE_STATEMENT_CONTINUE:
    _indent(out, indent);
    string_concat(out, "continue;\n");
    break;

  case CUBEC_NODE_STATEMENT_EMPTY:
    _indent(out, indent);
    string_concat(out, ";\n");
    break;

  /* --- Test (should NOT appear after desugar, but handle gracefully) --- */
  case CUBEC_NODE_STATEMENT_TEST: {
    cubec_statement_test_t st = (cubec_statement_test_t)n;
    _indent(out, indent);
    string_concat(out, "test ");
    string_concat(out, string_get(st->name));
    string_concat(out, " ");
    _write_stmt(a, out, st->body, indent);
    break;
  }

  /* --- Import (should NOT appear after desugar) --- */
  case CUBEC_NODE_STATEMENT_IMPORT: {
    cubec_statement_import_t imp = (cubec_statement_import_t)n;
    _indent(out, indent);
    string_concat(out, "import ");
    string_concat(out, _id_name(imp->module_name));
    string_concat(out, " from ");
    cubec_literal_string_t ls = (cubec_literal_string_t)imp->path;
    _write_str_literal(out, string_get(ls->value));
    string_concat(out, ";\n");
    break;
  }

  /* --- Comptime If / Foreach (should NOT appear after desugar) --- */
  case CUBEC_NODE_STATEMENT_COMPTIME_IF:
    _indent(out, indent);
    string_concat(out, "/* comptime if */\n");
    break;

  case CUBEC_NODE_STATEMENT_COMPTIME_FOREACH:
    _indent(out, indent);
    string_concat(out, "/* comptime foreach */\n");
    break;

  default:
    _indent(out, indent);
    string_concat(out, "/* unknown_stmt(#");
    {
      char buf[16];
      snprintf(buf, sizeof(buf), "%u", (unsigned)n->kind);
      string_concat(out, buf);
    }
    string_concat(out, ") */\n");
    break;
  }
}

/* ====================================================================
 *  Public entry point
 * ==================================================================== */

void cubec_ast_write(allocator_t allocator, node_t program, string_t out) {
  if (!program || !out) return;
  if (program->kind != CUBEC_NODE_PROGRAM) return;

  cubec_program_node_t prog = (cubec_program_node_t)program;
  if (!prog->statements) return;

  for (size_t i = 0; i < vec_get_size(prog->statements); i++) {
    node_t stmt = (node_t)vec_get(prog->statements, i);
    _write_stmt(allocator, out, stmt, 0);
  }
}
