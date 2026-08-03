#include "builder/cubec_writer.h"

#include "cubec/node.h"
#include "cubec/statement_function.h"
#include "cubec/statement_declaration.h"
#include "cubec/statement_struct.h"
#include "cubec/statement_enum.h"
#include "cubec/statement_union.h"
#include "cubec/statement_interface.h"
#include "cubec/statement_declaration_type.h"
#include "cubec/literal_identifier.h"
#include "cubec/literal_numeric.h"
#include "cubec/function_argument.h"
#include "cubec/declaration_variable.h"
#include "cubec/generic_param.h"
#include "cubec/expression_namespace_access.h"
#include "cubec/expression_generic_instantiation.h"
#include "cubec/expression_callable.h"
#include "cubec/expression_tuple.h"
#include "cubec/expression_qualifier.h"
#include <string.h>

/* Forward declarations */
static void write_type_expr(allocator_t allocator, string_t out, node_t type);
static void write_generic_params(allocator_t allocator, string_t out, vec_t params);

static const char *get_id_name(node_t id_node) {
  cubec_literal_identifier_t id = (cubec_literal_identifier_t)id_node;
  return string_get(id->value);
}

static void write_generic_params(allocator_t allocator, string_t out, vec_t params) {
  if (!params) return;
  size_t count = vec_get_size(params);
  if (count == 0) return;

  string_concat(out, "[");
  for (size_t i = 0; i < count; i++) {
    if (i > 0) string_concat(out, ", ");
    cubec_generic_param_t gp = vec_get(params, i);
    string_concat(out, get_id_name(gp->name));
    if (gp->value_type) {
      string_concat(out, ": ");
      write_type_expr(allocator, out, gp->value_type);
    }
  }
  string_concat(out, "]");
}

static void write_type_expr(allocator_t allocator, string_t out, node_t type) {
  if (!type) {
    string_concat(out, "void");
    return;
  }

  switch (type->kind) {
    case CUBEC_NODE_LITERAL_IDENTIFIER: {
      string_concat(out, get_id_name(type));
      break;
    }
    case CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS: {
      cubec_expression_namespace_access_t ns =
          (cubec_expression_namespace_access_t)type;
      write_type_expr(allocator, out, ns->host);
      string_concat(out, "::");
      string_concat(out, get_id_name((node_t)ns->field));
      break;
    }
    case CUBEC_NODE_DECLARATION_POINTER: {
      cubec_declaration_variable_t decl = (cubec_declaration_variable_t)type;
      write_type_expr(allocator, out, decl->type);
      string_concat(out, "*");
      break;
    }
    case CUBEC_NODE_DECLARATION_SLICE: {
      cubec_declaration_variable_t decl = (cubec_declaration_variable_t)type;
      string_concat(out, "[]");
      write_type_expr(allocator, out, decl->type);
      break;
    }
    case CUBEC_NODE_DECLARATION_ARRAY: {
      cubec_declaration_variable_t decl = (cubec_declaration_variable_t)type;
      string_concat(out, "[");
      if (decl->expression) {
        cubec_literal_numeric_t num = (cubec_literal_numeric_t)decl->expression;
        string_concat(out, string_get(num->value));
      }
      string_concat(out, "]");
      write_type_expr(allocator, out, decl->type);
      break;
    }
    case CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION: {
      cubec_expression_generic_instantiation_t gi =
          (cubec_expression_generic_instantiation_t)type;
      write_type_expr(allocator, out, gi->callee);
      string_concat(out, "[");
      if (gi->arguments) {
        size_t ac = vec_get_size(gi->arguments);
        for (size_t i = 0; i < ac; i++) {
          if (i > 0) string_concat(out, ", ");
          write_type_expr(allocator, out, vec_get(gi->arguments, i));
        }
      }
      string_concat(out, "]");
      break;
    }
    case CUBEC_NODE_EXPRESSION_CALLABLE: {
      cubec_expression_callable_t ft =
          (cubec_expression_callable_t)type;
      string_concat(out, "func(");
      if (ft->parameters) {
        size_t pc = vec_get_size(ft->parameters);
        for (size_t i = 0; i < pc; i++) {
          if (i > 0) string_concat(out, ", ");
          write_type_expr(allocator, out, vec_get(ft->parameters, i));
        }
      }
      if (ft->is_c_variadic) {
        if (ft->parameters && vec_get_size(ft->parameters) > 0)
          string_concat(out, ", ");
        string_concat(out, "...");
      }
      string_concat(out, ")");
      if (ft->return_type) {
        string_concat(out, " -> ");
        write_type_expr(allocator, out, ft->return_type);
      }
      break;
    }
    case CUBEC_NODE_EXPRESSION_TUPLE: {
      cubec_expression_tuple_t tt =
          (cubec_expression_tuple_t)type;
      string_concat(out, "<");
      if (tt->element_types) {
        size_t ec = vec_get_size(tt->element_types);
        for (size_t i = 0; i < ec; i++) {
          if (i > 0) string_concat(out, ", ");
          write_type_expr(allocator, out, vec_get(tt->element_types, i));
        }
      }
      string_concat(out, ">");
      break;
    }
    case CUBEC_NODE_EXPRESSION_QUALIFIER: {
      cubec_expression_qualifier_t tq =
          (cubec_expression_qualifier_t)type;
      if (tq->is_const) string_concat(out, "const ");
      if (tq->is_volatile) string_concat(out, "volatile ");
      write_type_expr(allocator, out, tq->type);
      break;
    }
    default: {
      string_concat(out, "/* unknown type */");
      break;
    }
  }
}

static void write_function_args(allocator_t allocator, string_t out, vec_t args) {
  string_concat(out, "(");
  if (args) {
    size_t count = vec_get_size(args);
    for (size_t i = 0; i < count; i++) {
      if (i > 0) string_concat(out, ", ");
      cubec_function_argument_t arg = vec_get(args, i);
      if (arg->is_rest) string_concat(out, "...");
      string_concat(out, get_id_name(arg->identifier));
      if (arg->type) {
        string_concat(out, ": ");
        write_type_expr(allocator, out, arg->type);
      }
    }
  }
  string_concat(out, ")");
}

static void write_extern_function(allocator_t allocator, string_t out,
                                   cubec_statement_function_t fn) {
  string_concat(out, "extern func ");
  string_concat(out, get_id_name(fn->name));
  if (fn->generic_params) {
    write_generic_params(allocator, out, fn->generic_params);
  }
  write_function_args(allocator, out, fn->arguments);
  if (fn->return_type) {
    string_concat(out, ": ");
    write_type_expr(allocator, out, fn->return_type);
  }
  string_concat(out, ";\n");
}

static void write_extern_variable(allocator_t allocator, string_t out,
                                   cubec_statement_declaration_t decl) {
  cubec_declaration_variable_t var = (cubec_declaration_variable_t)decl->declarator;
  string_concat(out, "extern var ");
  string_concat(out, get_id_name(var->identifier));
  if (var->type) {
    string_concat(out, ": ");
    write_type_expr(allocator, out, var->type);
  }
  string_concat(out, ";\n");
}

static void write_type_decl(allocator_t allocator, string_t out, node_t stmt) {
  switch (stmt->kind) {
    case CUBEC_NODE_STATEMENT_STRUCT: {
      cubec_statement_struct_t s = (cubec_statement_struct_t)stmt;
      string_concat(out, "export struct ");
      string_concat(out, get_id_name(s->name));
      if (s->generic_params)
        write_generic_params(allocator, out, s->generic_params);
      string_concat(out, ";\n");
      break;
    }
    case CUBEC_NODE_STATEMENT_ENUM: {
      cubec_statement_enum_t e = (cubec_statement_enum_t)stmt;
      string_concat(out, "export enum ");
      string_concat(out, get_id_name(e->name));
      string_concat(out, ";\n");
      break;
    }
    case CUBEC_NODE_STATEMENT_UNION: {
      cubec_statement_union_t u = (cubec_statement_union_t)stmt;
      string_concat(out, "export union ");
      string_concat(out, get_id_name(u->name));
      if (u->generic_params)
        write_generic_params(allocator, out, u->generic_params);
      string_concat(out, ";\n");
      break;
    }
    case CUBEC_NODE_STATEMENT_CUNION: {
      cubec_statement_union_t u = (cubec_statement_union_t)stmt;
      string_concat(out, "export cunion ");
      string_concat(out, get_id_name(u->name));
      string_concat(out, ";\n");
      break;
    }
    case CUBEC_NODE_STATEMENT_INTERFACE: {
      cubec_statement_interface_t iface = (cubec_statement_interface_t)stmt;
      string_concat(out, "export interface ");
      string_concat(out, get_id_name(iface->name));
      if (iface->generic_params)
        write_generic_params(allocator, out, iface->generic_params);
      string_concat(out, ";\n");
      break;
    }
    case CUBEC_NODE_STATEMENT_DECLARATION_TYPE: {
      cubec_statement_declaration_type_t td = (cubec_statement_declaration_type_t)stmt;
      string_concat(out, "export type ");
      string_concat(out, get_id_name(td->name));
      if (td->params)
        write_generic_params(allocator, out, td->params);
      string_concat(out, " = ");
      write_type_expr(allocator, out, td->type_value);
      string_concat(out, ";\n");
      break;
    }
    default:
      break;
  }
}

void cubec_write_interface(allocator_t allocator, node_t program, string_t out) {
  if (!program || program->kind != CUBEC_NODE_PROGRAM) return;

  cubec_program_node_t prog = (cubec_program_node_t)program;
  size_t count = vec_get_size(prog->statements);

  /* Emit extern declarations for exported functions and variables,
     and forward declarations for exported types */
  for (size_t i = 0; i < count; i++) {
    node_t stmt = vec_get(prog->statements, i);

    switch (stmt->kind) {
      case CUBEC_NODE_STATEMENT_FUNCTION: {
        cubec_statement_function_t fn = (cubec_statement_function_t)stmt;
        if (fn->is_export) {
          write_extern_function(allocator, out, fn);
        }
        break;
      }
      case CUBEC_NODE_STATEMENT_DECLARATION: {
        cubec_statement_declaration_t decl = (cubec_statement_declaration_t)stmt;
        if (decl->is_export) {
          write_extern_variable(allocator, out, decl);
        }
        break;
      }
      case CUBEC_NODE_STATEMENT_STRUCT:
      case CUBEC_NODE_STATEMENT_ENUM:
      case CUBEC_NODE_STATEMENT_UNION:
      case CUBEC_NODE_STATEMENT_CUNION:
      case CUBEC_NODE_STATEMENT_INTERFACE:
      case CUBEC_NODE_STATEMENT_DECLARATION_TYPE: {
        cubec_statement_struct_t s = (cubec_statement_struct_t)stmt;
        if (s->is_export) {
          write_type_decl(allocator, out, stmt);
        }
        break;
      }
      default:
        break;
    }
  }
}
