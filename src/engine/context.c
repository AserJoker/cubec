#include "engine/context.h"
#include "engine/module.h"
#include "engine/void_type.h"
#include "engine/bool_type.h"
#include "engine/char_type.h"
#include "engine/str_type.h"
#include "engine/nil_type.h"
#include "engine/integer_type.h"
#include "engine/float_type.h"
#include "engine/pointer_type.h"
#include "engine/tuple_type.h"
#include "engine/array_type.h"
#include "engine/slice_type.h"
#include "engine/callable_type.h"
#include "engine/struct_instance.h"
#include "engine/union_instance.h"
#include "engine/cunion_instance.h"
#include "engine/enum_instance.h"
#include "engine/value.h"
#include "engine/scope.h"
#include "core/rbtree.h"
#include "core/vec.h"
#include "cubec/program.h"
#include "cubec/token.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void _context_init(void *self, allocator_t allocator, void *arg) {
  (void)arg;
  context_t ctx = (context_t)self;
  memset(ctx, 0, sizeof(struct context));
  ctx->allocator = allocator;

  diagnostic_list_init_t dl_init = {.output = NULL};
  ctx->diagnostics = (diagnostic_list_t)allocator_create(
      allocator, &g_diagnostic_list_type, &dl_init);

  strmap_init_t sm_init = {.value_auto_dispose = true};
  ctx->modules =
      (strmap_t)allocator_create(allocator, &g_strmap_type, &sm_init);

  ctx->global_scope = scope_create(allocator, SCOPE_GLOBAL, NULL, NULL);
  ctx->root_scope = NULL;
  ctx->current_scope = NULL;

  rbtree_init_t types_init = {.auto_dispose = true};
  ctx->types = (rbtree_t)allocator_create(allocator, &g_rbtree_type, &types_init);

  rbtree_init_t strings_init = {.auto_dispose = true};
  ctx->strings = (rbtree_t)allocator_create(allocator, &g_rbtree_type, &strings_init);

  void_type_register(ctx);
  bool_type_register(ctx);
  char_type_register(ctx);
  str_type_register(ctx);
  nil_type_register(ctx);
  integer_types_register(ctx);
  float_types_register(ctx);
}

static void _context_dispose(void *self, allocator_t allocator) {
  context_t ctx = (context_t)self;
  (void)allocator;
  /* Free modules first — module_dispose removes root_scope from global_scope's
   * children, so global_scope must still be alive at this point. */
  allocator_free(allocator, &ctx->modules);
  allocator_free(allocator, &ctx->types);
  allocator_free(allocator, &ctx->strings);
  allocator_free(allocator, &ctx->global_scope);
  allocator_free(allocator, &ctx->diagnostics);
}

type_t g_context_type = {
    .size = sizeof(struct context),
    .name = "cubec.engine.context",
    .init = (type_init_fn_t)_context_init,
    .dispose = (type_dispose_fn_t)_context_dispose,
};

context_t context_create(allocator_t allocator) {
  return (context_t)allocator_create(allocator, &g_context_type, NULL);
}

void context_dispose(context_t ctx) {
  allocator_free(ctx->allocator, &ctx);
}

int context_get_error_count(context_t ctx) {
  if (!ctx) return 0;
  return (int)diagnostic_list_get_error_count(ctx->diagnostics);
}

module_t context_get_module(context_t ctx, const char *abs_path) {
  return (module_t)strmap_find(ctx->modules, abs_path);
}

/* ------------------------------------------------------------------
 *  File I/O helper
 * ------------------------------------------------------------------ */

static char *_read_file(const char *path, size_t *out_len) {
  FILE *f = fopen(path, "rb");
  if (!f)
    return NULL;
  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *buf = malloc((size_t)len + 1);
  if (!buf) {
    fclose(f);
    return NULL;
  }
  size_t n = fread(buf, 1, (size_t)len, f);
  buf[n] = '\0';
  fclose(f);
  if (out_len)
    *out_len = n;
  return buf;
}

/* ------------------------------------------------------------------
 *  Path resolution
 * ------------------------------------------------------------------ */

/**
 * @brief Resolve an import path to an absolute file path.
 *
 * - Relative paths (starting with "./" or "../") are resolved relative to the
 *   importing module's directory.
 * - Absolute paths are used as-is.
 * - Bare module names (e.g., "std") are NOT supported yet — they require
 *   package/library search paths which will be handled later.
 *
 * @param ctx         Compiler context
 * @param import_path Import path from source (e.g., "./io", "/abs/path")
 * @return Malloc'd absolute path string, or NULL on failure
 */
static char *_resolve_import_path(context_t ctx, const char *import_path) {
  if (!import_path || import_path[0] == '\0')
    return NULL;

  /* Check if path is relative (starts with ./ or ../) */
  bool is_relative =
      (import_path[0] == '.' &&
       (import_path[1] == '/' || (import_path[1] == '.' && import_path[2] == '/')));

  /* Check if path is absolute */
  bool is_absolute = (import_path[0] == '/' || import_path[0] == '\\'
#ifdef _WIN32
                      || (import_path[0] && import_path[1] == ':')
#endif
  );

  /* Bare module names (e.g., "std") are not supported */
  if (!is_relative && !is_absolute)
    return NULL;

  char *resolved = NULL;

  if (is_relative) {
    /* Resolve relative to the importing module's directory */
    if (ctx->root_scope && ctx->root_scope->owner) {
      module_t current_mod = (module_t)ctx->root_scope->owner;
      const char *current_file = current_mod->filename;
      if (current_file) {
        /* Find last '/' to get directory */
        const char *last_slash = strrchr(current_file, '/');
#ifdef _WIN32
        const char *last_backslash = strrchr(current_file, '\\');
        if (!last_slash || (last_backslash && last_backslash > last_slash))
          last_slash = last_backslash;
#endif
        if (last_slash) {
          size_t dir_len = (size_t)(last_slash - current_file) + 1;
          size_t path_len = strlen(import_path);
          resolved = malloc(dir_len + path_len + 1);
          if (!resolved)
            return NULL;
          memcpy(resolved, current_file, dir_len);
          memcpy(resolved + dir_len, import_path, path_len);
          resolved[dir_len + path_len] = '\0';
        }
      }
    }
    /* Fallback: treat as relative to cwd */
  }

  if (!resolved) {
    /* Absolute path — append .cubec extension if missing */
    size_t path_len = strlen(import_path);
    bool has_ext = (path_len > 6 && strcmp(import_path + path_len - 6, ".cubec") == 0);
    size_t ext_len = has_ext ? 0 : 6;
    resolved = malloc(path_len + ext_len + 1);
    if (!resolved)
      return NULL;
    memcpy(resolved, import_path, path_len);
    if (!has_ext) {
      memcpy(resolved + path_len, ".cubec", 6);
    }
    resolved[path_len + ext_len] = '\0';
  }

  /* Normalize to absolute path */
#ifdef _WIN32
  char abs_buf[_MAX_PATH];
  char *abs = _fullpath(abs_buf, resolved, _MAX_PATH);
  char *result = abs ? strdup(abs) : NULL;
#else
  char *result = realpath(resolved, NULL);
#endif
  free(resolved);
  return result;
}

/* ------------------------------------------------------------------
 *  context_import
 * ------------------------------------------------------------------ */

module_t context_import(context_t ctx, const char *import_path) {
  /* 1. Resolve import path to absolute file path */
  char *abs_path = _resolve_import_path(ctx, import_path);
  if (!abs_path)
    return NULL;

  /* 2. Check cache */
  module_t existing = context_get_module(ctx, abs_path);
  if (existing) {
    free(abs_path);
    return existing;
  }

  /* 3. Read source file */
  char *source = _read_file(abs_path, NULL);
  if (!source) {
    free(abs_path);
    return NULL;
  }

  /* 4. Tokenize */
  vec_t tokens = resolve_token_list(ctx, abs_path, source);
  if (!tokens) {
    free(source);
    free(abs_path);
    return NULL;
  }

  /* 5. Parse to AST */
  size_t pos = 0;
  node_t program = read_program_node(ctx, tokens, &pos, abs_path);
  if (!program) {
    allocator_free(ctx->allocator, &tokens);
    free(source);
    free(abs_path);
    return NULL;
  }

  /* 6. Create module (takes ownership of source, tokens, program) */
  module_t mod = module_create(ctx->allocator, ctx->global_scope, abs_path,
                               source, tokens, program);

  /* 7. Register in modules map (strmap with value_auto_dispose=true) */
  strmap_insert(ctx->modules, abs_path, mod);

  free(abs_path);
  return mod;
}

/* ------------------------------------------------------------------
 *  Scope stack
 * ------------------------------------------------------------------ */

void context_push_scope(context_t ctx, scope_t scope) {
  if (!ctx || !scope)
    return;
  ctx->root_scope = scope;
  ctx->current_scope = scope;
}

void context_pop_scope(context_t ctx) {
  if (!ctx || !ctx->current_scope)
    return;
  ctx->current_scope = ctx->current_scope->parent;
}

/* ------------------------------------------------------------------
 *  Value creation with generic instantiation
 * ------------------------------------------------------------------ */

/**
 * @brief Ensure a generic type has its implements rbtree initialized.
 *
 * Non-generic types do not need an implements rbtree — they have a single
 * instance keyed by type->instance.hash. Only generic types need to track
 * multiple instances.
 */
static void _ensure_implements(context_t ctx, stype_t type) {
  if (!type->params)
    return; /* non-generic types don't need implements */
  if (type->implements)
    return;
  rbtree_init_t rbi = {.auto_dispose = true};
  type->implements =
      (rbtree_t)allocator_create(ctx->allocator, &g_rbtree_type, &rbi);
}

/**
 * @brief Instantiate a generic type with the given args.
 *
 * Computes instance hash from generic_args (vec of value_t),
 * looks up in type->implements rbtree.
 * Creates a new instance entry if not found.
 *
 * For non-generic types, returns type->instance.hash (the default instance).
 *
 * @return The instance hash.
 */
static uint64_t _instantiate(context_t ctx, stype_t type, vec_t generic_args) {
  /* Non-generic types: return the default instance hash */
  if (!type->params)
    return type->instance.hash;

  _ensure_implements(ctx, type);

  /* Generic type without args: return the template hash */
  if (!generic_args)
    return type->instance.hash;

  /* Build generic_arg_hashes from value_t args */
  vec_init_t vi = {.auto_dispose = false};
  vec_t generic_arg_hashes =
      (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);

  size_t n = vec_get_size(generic_args);
  for (size_t i = 0; i < n; i++) {
    value_t arg = (value_t)vec_get(generic_args, i);
    uint64_t arg_hash = value_data_hash((context_t)ctx, arg->stype, arg->type_hash,
                                        arg->data);
    vec_push(generic_arg_hashes, (void *)(uintptr_t)arg_hash);
  }

  /* Compute instance hash */
  uint64_t instance_hash =
      stype_compute_named_type_hash(type->type_kind, generic_arg_hashes,
                                    NULL, NULL);
  allocator_free(ctx->allocator, &generic_arg_hashes);

  /* Check if already instantiated */
  void *existing = rbtree_find(type->implements, instance_hash);
  if (existing)
    return instance_hash;

  /* Create new instance stub — field layout filled during type resolution */
  rbtree_insert(type->implements, instance_hash, (void *)(uintptr_t)1);

  return instance_hash;
}

value_t context_create_value(context_t ctx, stype_t type, vec_t generic_args,
                             const void *data,
                             bool is_export, bool is_exportlib, bool is_extern,
                             bool is_builtin, bool is_comptime, bool is_using) {
  /* Ensure instance exists (instantiate generic if needed) */
  uint64_t type_hash = _instantiate(ctx, type, generic_args);

  value_t val = value_create(ctx->allocator, NULL, is_export, is_exportlib,
                             is_extern, is_builtin, is_comptime, is_using);
  val->stype = type;
  val->type_hash = type_hash;

  /* Copy source data into owned raw buffer */
  if (data && type->instance.size > 0) {
    val->data = allocator_alloc(ctx->allocator, (size_t)type->instance.size);
    memcpy(val->data, data, (size_t)type->instance.size);
  }

  return val;
}

/* ------------------------------------------------------------------
 *  Per-kind convenience wrappers
 * ------------------------------------------------------------------ */

value_t context_create_int_value(context_t ctx, stype_t type, uint64_t val) {
  return context_create_value(ctx, type, NULL, &val,
                              false, false, false, false, false, false);
}

value_t context_create_float_value(context_t ctx, stype_t type, double val) {
  return context_create_value(ctx, type, NULL, &val,
                              false, false, false, false, false, false);
}

value_t context_create_bool_value(context_t ctx, stype_t type, bool val) {
  return context_create_value(ctx, type, NULL, &val,
                              false, false, false, false, false, false);
}

value_t context_create_char_value(context_t ctx, stype_t type, uint8_t val) {
  return context_create_value(ctx, type, NULL, &val,
                              false, false, false, false, false, false);
}

value_t context_create_str_value(context_t ctx, stype_t type, const char *val) {
  /* String values store a string_id (uint64_t) in the raw buffer.
   * The actual string is interned in ctx->strings rbtree. */
  (void)val; /* TODO: implement context_intern_string */
  uint64_t string_id = 0;
  return context_create_value(ctx, type, NULL, &string_id,
                              false, false, false, false, false, false);
}

value_t context_create_nil_value(context_t ctx, stype_t type) {
  return context_create_value(ctx, type, NULL, NULL,
                              false, false, false, false, false, false);
}

/* ------------------------------------------------------------------
 *  Value data hash — dispatches to per-type hash_value implementations
 * ------------------------------------------------------------------ */

uint64_t value_data_hash(context_t ctx, stype_t type, uint64_t type_hash,
                         const void *data) {
  if (!type) return 0;

  switch (type->type_kind) {
  case TYPE_VOID:
    return void_type_hash_value(type, type_hash, data);
  case TYPE_BOOL:
    return bool_type_hash_value(type, type_hash, data);
  case TYPE_CHAR:
    return char_type_hash_value(type, type_hash, data);
  case TYPE_I8:
  case TYPE_I16:
  case TYPE_I32:
  case TYPE_I64:
  case TYPE_U8:
  case TYPE_U16:
  case TYPE_U32:
  case TYPE_U64:
    return integer_type_hash_value(type, type_hash, data);
  case TYPE_F16:
  case TYPE_F32:
  case TYPE_F64:
    return float_type_hash_value(type, type_hash, data);
  case TYPE_STR:
    return str_type_hash_value(ctx, type, type_hash, data);
  case TYPE_NIL:
    return nil_type_hash_value(type, type_hash, data);
  case TYPE_POINTER:
    return pointer_type_hash_value(type, type_hash, data);
  case TYPE_TUPLE:
    return tuple_type_hash_value(ctx, type, type_hash, data);
  case TYPE_ARRAY:
    return array_type_hash_value(ctx, type, type_hash, data);
  case TYPE_SLICE:
    return slice_type_hash_value(type, type_hash, data);
  case TYPE_CALLABLE:
    return callable_type_hash_value(type, type_hash, data);
  case TYPE_STRUCT:
    return struct_instance_hash_value(ctx, type, type_hash, data);
  case TYPE_UNION:
    return union_instance_hash_value(ctx, type, type_hash, data);
  case TYPE_CUNION:
    return cunion_instance_hash_value(type, type_hash, data);
  case TYPE_ENUM:
    return enum_instance_hash_value(type, type_hash, data);
  case TYPE_INTERFACE:
  case TYPE_TYPE_ALIAS:
    return type_hash;
  }

  return type_hash;
}
