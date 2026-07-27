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
#include "cubec/statement_struct.h"
#include "cubec/statement_enum.h"
#include "cubec/statement_union.h"
#include "cubec/statement_cunion.h"
#include "cubec/statement_declaration.h"
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
                        const char *module_hash);
c_ir_node_t lower_stmt(allocator_t allocator, context_t ctx, node_t node,
                        const char *module_hash);

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

        /* Forward declaration for struct/union/cunion */
        if (kind == TYPE_STRUCT || kind == TYPE_UNION || kind == TYPE_CUNION) {
          c_ir_node_t fwd = (c_ir_node_t)c_ir_forward_decl_create(
              allocator, string_get(mangled), sym->location);
          vec_push(unit->forward_decls, fwd);
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

      /* TODO: full function lowering — for now, create placeholder */
      break;
    }

    case SYMBOL_VARIABLE: {
      /* Global variable */
      if (sym->variable.is_comptime) break;
      /* TODO: generate variable_decl */
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

  /* Lower global scope declarations (types, enums, etc.) */
  lower_scope_declarations(allocator, ctx, unit, module_hash);

  /* TODO: Lower function bodies */

  allocator_free(allocator, &module_hash_str);
  return unit;
}
