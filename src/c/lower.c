#include "c/lower.h"
#include "c/c_ir.h"
#include "c/c_type.h"
#include "c/mangle.h"
#include "c/c_ir_unit.h"
#include "c/c_ir_function.h"
#include "c/c_ir_variable.h"
#include "c/c_ir_enum.h"
#include "c/c_ir_typedef.h"
#include "c/c_ir_forward_decl.h"
#include "c/c_ir_include.h"
#include "c/c_ir_stmt_block.h"
#include "cubec/node.h"
#include "cubec/statement_function.h"
#include "cubec/function_argument.h"
#include "cubec/statement_struct.h"
#include "cubec/statement_enum.h"
#include "cubec/statement_union.h"
#include "cubec/statement_cunion.h"
#include "cubec/statement_declaration.h"
#include "cubec/statement_defer.h"
#include "cubec/literal_identifier.h"
#include "engine/semantic_type.h"
#include "engine/symbol.h"
#include "engine/scope.h"
#include "engine/context.h"
#include "core/node.h"
#include <string.h>

/* Forward declarations (defined in lower_expr.c and lower_stmt.c) */
c_type_t lower_type(allocator_t allocator, semantic_type_t type,
                     const char *module_hash);
c_ir_node_t lower_expr(allocator_t allocator, context_t ctx, node_t node,
                        const char *module_hash, c_ir_unit_t unit);
c_ir_node_t lower_stmt(allocator_t allocator, context_t ctx, node_t node,
                        const char *module_hash, vec_t defer_stack);

/**
 * @brief Lower top-level declarations from the global scope into C IR.
 */
static void lower_scope_declarations(allocator_t allocator, context_t ctx,
                                      c_ir_unit_t unit, const char *module_hash) {
  scope_t scope = ctx->global_scope;
  vec_t symbols = scope_get_symbols(scope);
  size_t count = vec_get_size(symbols);

  for (size_t i = 0; i < count; i++) {
    struct symbol *sym = vec_get(symbols, i);

    switch (sym->kind) {
    case SYMBOL_TYPE: {
      semantic_type_t type = sym->type.type;
      if (!type) break;
      enum type_kind kind = semantic_type_get_kind(type);
      const char *name = semantic_type_get_name(type);

      /* Interfaces and generic params produce no C code */
      if (kind == TYPE_INTERFACE || kind == TYPE_GENERIC_PARAM ||
          kind == TYPE_GENERIC_PACK || kind == TYPE_ERROR) {
        break;
      }

      if (name) {
        string_t mangled = mangle_name(allocator, module_hash, name);

        /* Forward declaration for struct/union/cunion/slice */
        if (kind == TYPE_STRUCT || kind == TYPE_UNION || kind == TYPE_CUNION ||
            kind == TYPE_SLICE) {
          if (kind == TYPE_SLICE) {
            /* Slice: generate full struct definition with element pointer type */
            type_impl_t impl = semantic_type_get_impl(type);
            c_type_t elem_c_type = lower_type(allocator, impl->slice.element,
                                                module_hash);
            /* Build body: "elem_type* ptr;\n    size_t start;\n    size_t length;\n" */
            string_t body = allocator_create(allocator, &g_string_type,
                                              &(string_init_t){.str = ""});
            string_concat(body, string_get(elem_c_type->left));
            if (elem_c_type->right && strlen(string_get(elem_c_type->right)) > 0) {
              string_concat(body, " ");
              string_concat(body, string_get(elem_c_type->right));
            }
            string_concat(body, "* ptr;\n    size_t start;\n    size_t length;\n");
            c_type_dispose(allocator, &elem_c_type);

            c_ir_node_t fwd = (c_ir_node_t)c_ir_forward_decl_create_with_body(
                allocator, string_get(mangled), string_get(body), sym->location);
            vec_push(unit->forward_decls, fwd);
            allocator_free(allocator, &body);
          } else {
            c_ir_node_t fwd = (c_ir_node_t)c_ir_forward_decl_create(
                allocator, string_get(mangled), sym->location);
            vec_push(unit->forward_decls, fwd);
          }
        }

        /* Typedef */
        if (kind == TYPE_STRUCT || kind == TYPE_UNION || kind == TYPE_CUNION ||
            kind == TYPE_TUPLE || kind == TYPE_SLICE) {
          /* Build the underlying C type */
          c_type_t c_type = NULL;
          type_impl_t impl = semantic_type_get_impl(type);

          if (kind == TYPE_STRUCT || kind == TYPE_UNION || kind == TYPE_CUNION) {
            /* Self-referencing typedef: typedef struct m3a7_X m3a7_X */
            c_type = c_type_primitive(allocator, string_get(mangled));
          } else {
            c_type = lower_type(allocator, type, module_hash);
          }

          c_ir_node_t td = (c_ir_node_t)c_ir_typedef_create(
              allocator, c_type, string_get(mangled), sym->location);
          vec_push(unit->typedefs, td);
          allocator_free(allocator, &mangled);
          break;
        }

        /* Enum */
        if (kind == TYPE_ENUM) {
          type_impl_t impl = semantic_type_get_impl(type);
          c_type_t backing = impl->enum_type.backing_type
              ? lower_type(allocator, impl->enum_type.backing_type, module_hash)
              : NULL;

          vec_t items = allocator_create(allocator, &g_vec_type,
                                           &(vec_init_t){.auto_dispose = false});
          size_t item_count = impl->enum_type.items
              ? vec_get_size(impl->enum_type.items) : 0;
          for (size_t j = 0; j < item_count; j++) {
            struct symbol *item_sym = vec_get(impl->enum_type.items, j);
            string_t item_name = mangle_enum_item(allocator, module_hash,
                                                    name, item_sym->name);
            char val_buf[32];
            snprintf(val_buf, sizeof(val_buf), "%lld", item_sym->enum_item.value);
            c_ir_enum_item_t ei = c_ir_enum_item_create(allocator,
                                                          string_get(item_name),
                                                          val_buf);
            vec_push(items, ei);
            allocator_free(allocator, &item_name);
          }

          c_ir_node_t enum_def = (c_ir_node_t)c_ir_enum_def_create(
              allocator, string_get(mangled), backing, items, sym->location);
          vec_push(unit->enum_defs, enum_def);
          allocator_free(allocator, &mangled);
          break;
        }

        allocator_free(allocator, &mangled);
      }
      break;
    }

    case SYMBOL_FUNCTION: {
      /* Skip comptime and builtin functions */
      if (sym->function.is_comptime || sym->is_builtin) break;
      if (!sym->function.type) break;

      type_impl_t impl = semantic_type_get_impl(sym->function.type);

      /* Build parameter list */
      vec_t params = allocator_create(allocator, &g_vec_type,
                                       &(vec_init_t){.auto_dispose = false});
      size_t param_count = impl->function.params
          ? vec_get_size(impl->function.params) : 0;

      cubec_statement_function_t fn_ast =
          (cubec_statement_function_t)sym->function.ast_node;

      for (size_t j = 0; j < param_count; j++) {
        semantic_type_t param_type = vec_get(impl->function.params, j);
        c_type_t c_type = lower_type(allocator, param_type, module_hash);
        const char *param_name = "_";
        if (fn_ast && fn_ast->arguments && j < vec_get_size(fn_ast->arguments)) {
          cubec_function_argument_t arg = vec_get(fn_ast->arguments, j);
          if (arg->identifier) {
            cubec_literal_identifier_t id = (cubec_literal_identifier_t)arg->identifier;
            param_name = string_get(id->value);
          }
        }
        c_ir_param_t p = c_ir_param_create(allocator, c_type, param_name);
        vec_push(params, p);
      }

      /* Return type */
      c_type_t ret_type = lower_type(allocator, impl->function.return_type,
                                       module_hash);

      /* Mangled function name */
      string_t mangled = mangle_name(allocator, module_hash, sym->name);

      bool is_export = sym->is_export;
      bool is_inline = fn_ast ? fn_ast->is_inline : false;

      if (fn_ast && fn_ast->body) {
        /* Create defer stack for this function */
        vec_t defer_stack = allocator_create(allocator, &g_vec_type,
                                               &(vec_init_t){.auto_dispose = false});

        /* Function definition */
        c_ir_node_t body = lower_stmt(allocator, ctx, fn_ast->body,
                                        module_hash, defer_stack);

        /* Append deferred cleanup at end of function body (LIFO order) */
        size_t defer_count = vec_get_size(defer_stack);
        if (defer_count > 0 && body &&
            c_ir_get_kind(body) == C_IR_STMT_BLOCK) {
          c_ir_stmt_block_t block = (c_ir_stmt_block_t)body;
          for (size_t d = defer_count; d > 0; d--) {
            node_t defer_node = vec_get(defer_stack, d - 1);
            cubec_statement_defer_t df = (cubec_statement_defer_t)defer_node;
            c_ir_node_t defer_body = lower_stmt(allocator, ctx, df->body,
                                                  module_hash, NULL);
            if (defer_body) vec_push(block->statements, defer_body);
          }
        }

        allocator_free(allocator, &defer_stack);
        c_ir_node_t def = (c_ir_node_t)c_ir_function_def_create(
            allocator, ret_type, string_get(mangled), params,
            !is_export, is_inline, false, false, body, sym->location);
        vec_push(unit->function_defs, def);

        /* Export functions → declaration in .h */
        if (is_export) {
          vec_t decl_params = allocator_create(allocator, &g_vec_type,
                                                &(vec_init_t){.auto_dispose = false});
          size_t pc = impl->function.params
              ? vec_get_size(impl->function.params) : 0;
          for (size_t j = 0; j < pc; j++) {
            semantic_type_t pt = vec_get(impl->function.params, j);
            c_type_t ct = lower_type(allocator, pt, module_hash);
            const char *pn = "_";
            if (fn_ast->arguments && j < vec_get_size(fn_ast->arguments)) {
              cubec_function_argument_t arg = vec_get(fn_ast->arguments, j);
              if (arg->identifier) {
                cubec_literal_identifier_t id = (cubec_literal_identifier_t)arg->identifier;
                pn = string_get(id->value);
              }
            }
            c_ir_param_t p = c_ir_param_create(allocator, ct, pn);
            vec_push(decl_params, p);
          }
          c_type_t decl_ret = lower_type(allocator, impl->function.return_type,
                                           module_hash);
          c_ir_node_t decl = (c_ir_node_t)c_ir_function_decl_create(
              allocator, decl_ret, string_get(mangled), decl_params,
              false, is_inline, false, false, sym->location);
          vec_push(unit->function_decls, decl);
        }
      } else {
        /* Extern/interface → declaration only */
        c_ir_node_t decl = (c_ir_node_t)c_ir_function_decl_create(
            allocator, ret_type, string_get(mangled), params,
            false, false, false, false, sym->location);
        vec_push(unit->function_decls, decl);
      }

      allocator_free(allocator, &mangled);
      break;
    }

    case SYMBOL_VARIABLE: {
      /* Global variable */
      if (sym->variable.is_comptime || sym->is_builtin) break;
      if (!sym->variable.type) break;

      c_type_t c_type = lower_type(allocator, sym->variable.type, module_hash);
      if (!sym->variable.is_mutable) c_type_const(allocator, c_type);

      string_t mangled = mangle_name(allocator, module_hash, sym->name);
      c_ir_node_t vd = (c_ir_node_t)c_ir_variable_decl_create(
          allocator, c_type, string_get(mangled),
          NULL,   /* init — TODO: from AST */
          !sym->is_export,  /* is_static */
          false,  /* is_extern */
          sym->location);
      vec_push(unit->variable_decls, vd);
      allocator_free(allocator, &mangled);
      break;
    }

    default:
      break;
    }
  }
}

/**
 * @brief Lower the program AST into a C IR compilation unit.
 */
c_ir_unit_t lower_program(allocator_t allocator, context_t ctx, node_t program) {
  if (!program || program->kind != CUBEC_NODE_PROGRAM) return NULL;

  /* Compute module hash from filename */
  const char *filename = ctx->current_file ? ctx->current_file : "unknown";
  string_t module_hash_str = mangle_module_hash(allocator, filename);
  const char *module_hash = string_get(module_hash_str);

  /* Create the compilation unit */
  location_t loc = program->location;
  c_ir_unit_t unit = c_ir_unit_create(allocator, filename, module_hash, loc);

  /* Add standard includes */
  c_ir_node_t stdint_h = (c_ir_node_t)c_ir_include_create(
      allocator, "stdint.h", true, loc);
  vec_push(unit->includes, stdint_h);
  c_ir_node_t stdbool_h = (c_ir_node_t)c_ir_include_create(
      allocator, "stdbool.h", true, loc);
  vec_push(unit->includes, stdbool_h);
  c_ir_node_t stdlib_h = (c_ir_node_t)c_ir_include_create(
      allocator, "stdlib.h", true, loc);
  vec_push(unit->includes, stdlib_h);
  c_ir_node_t stdio_h = (c_ir_node_t)c_ir_include_create(
      allocator, "stdio.h", true, loc);
  vec_push(unit->includes, stdio_h);

  /* Lower global scope declarations (types, enums, etc.) */
  lower_scope_declarations(allocator, ctx, unit, module_hash);

  /* TODO: Lower function bodies */

  allocator_free(allocator, &module_hash_str);
  return unit;
}
