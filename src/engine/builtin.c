#include "engine/builtin.h"
#include "engine/checker.h"
#include "engine/symbol.h"
#include "engine/type_hash.h"
#include "core/allocator.h"
#include "core/vec.h"
#include <string.h>

/* ===== builtin_entry type ===== */

static void _builtin_entry_init(void *self, allocator_t allocator, void *arg) {
  (void)allocator;
  builtin_entry_t entry = (builtin_entry_t)self;
  memset(entry, 0, sizeof(struct builtin_entry));
  if (arg) {
    /* arg points to a temporary struct with initialization data */
    builtin_entry_t src = (builtin_entry_t)arg;
    entry->name = src->name;
    entry->kind = src->kind;
    entry->type = src->type;
    entry->dispatch = src->dispatch;
  }
}

type_t g_builtin_entry_type = {
    .size = sizeof(struct builtin_entry),
    .name = "cubec.engine.builtin_entry",
    .init = (type_init_fn_t)_builtin_entry_init,
    .dispose = NULL,
};

/* ===== builtin_table lifecycle ===== */

static void _builtin_table_init(void *self, allocator_t allocator, void *arg) {
  (void)arg;
  builtin_table_t table = (builtin_table_t)self;
  memset(table, 0, sizeof(struct builtin_table));
  table->allocator = allocator;
  strmap_init_t si = {.value_auto_dispose = false};
  table->entries = (strmap_t)allocator_create(allocator, &g_strmap_type, &si);
  vec_init_t vi = {.auto_dispose = true};
  table->all_entries = (vec_t)allocator_create(allocator, &g_vec_type, &vi);
}

static void _builtin_table_dispose(void *self, allocator_t allocator) {
  builtin_table_t table = (builtin_table_t)self;
  allocator_free(allocator, &table->entries);
  allocator_free(allocator, &table->all_entries);
}

static type_t g_builtin_table_type = {
    .size = sizeof(struct builtin_table),
    .name = "cubec.engine.builtin_table",
    .init = (type_init_fn_t)_builtin_table_init,
    .dispose = (type_dispose_fn_t)_builtin_table_dispose,
};

builtin_table_t builtin_table_create(allocator_t allocator) {
  return (builtin_table_t)allocator_create(allocator, &g_builtin_table_type, NULL);
}

void builtin_table_dispose(builtin_table_t table, allocator_t allocator) {
  allocator_free(allocator, &table);
}

/* ===== registration ===== */

void builtin_table_register(builtin_table_t table, const char *name,
                            enum builtin_kind kind, semantic_type_t type,
                            enum builtin_dispatch dispatch) {
  struct builtin_entry init_data = {
      .name = name,
      .kind = kind,
      .type = type,
      .dispatch = dispatch,
  };
  builtin_entry_t entry =
      (builtin_entry_t)allocator_create(table->allocator, &g_builtin_entry_type, &init_data);
  vec_push(table->all_entries, entry);
  strmap_insert(table->entries, name, entry);
}

/* ===== query ===== */

builtin_entry_t builtin_table_lookup(builtin_table_t table, const char *name) {
  if (!table || !name) return NULL;
  void *found = strmap_find(table->entries, name);
  return found ? (builtin_entry_t)found : NULL;
}

/* ===== defaults ===== */

void builtin_table_init_defaults(builtin_table_t table, struct checker *ctx) {
  /* assert(condition: bool): void */
  {
    vec_init_t vi = {.auto_dispose = false};
    vec_t params = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);
    vec_push(params, ctx->builtin_bool);
    semantic_type_t assert_type = semantic_type_create_function(
        ctx->allocator, ctx->builtin_void, params, false);
    type_hash_ensure(assert_type);
    vec_push(ctx->all_types, assert_type);
    builtin_table_register(table, "assert",
                           BUILTIN_FUNC, assert_type, BUILTIN_DISPATCH_ASSERT);
  }

  /* builtin type Tuple[...Args] — variadic generic tuple */
  {
    semantic_type_t tuple_type = semantic_type_create_named(
        ctx->allocator, "Tuple", TYPE_STRUCT);
    /* No fields at template level — populated at instantiation */
    tuple_type->impl->struct_type.fields = NULL;
    type_hash_ensure(tuple_type);
    vec_push(ctx->all_types, tuple_type);
    builtin_table_register(table, "Tuple",
                           BUILTIN_TYPE, tuple_type, BUILTIN_DISPATCH_TUPLE);
  }

  /* builtin func length[T](list: T): u64 */
  {
    /* Create generic param T at index 0 */
    semantic_type_t t_param = semantic_type_create_generic_param(
        ctx->allocator, "T", 0, NULL, false);
    type_hash_ensure(t_param);
    vec_push(ctx->all_types, t_param);

    vec_init_t vi = {.auto_dispose = false};
    vec_t params = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);
    vec_push(params, t_param);
    semantic_type_t length_type = semantic_type_create_function(
        ctx->allocator, ctx->builtin_u64, params, false);
    type_hash_ensure(length_type);
    vec_push(ctx->all_types, length_type);
    builtin_table_register(table, "length",
                           BUILTIN_FUNC, length_type, BUILTIN_DISPATCH_LENGTH);
  }

  /* builtin func get[N: u64, ...Args](tuple: Tuple[...Args]): Args[N]
     Return type is resolved dynamically by the type checker based on
     the compile-time constant N and the concrete Tuple type.
     N: u64 is a value generic param; index is no longer a function param. */
  {
    /* Generic params: N: u64 (value param at index 0), ...Args (pack at index 1) */
    semantic_type_t n_param = semantic_type_create_generic_param(
        ctx->allocator, "N", 0, ctx->builtin_u64, true);
    type_hash_ensure(n_param);
    vec_push(ctx->all_types, n_param);

    semantic_type_t args_param = semantic_type_create_generic_pack(
        ctx->allocator, "Args", 1);
    type_hash_ensure(args_param);
    vec_push(ctx->all_types, args_param);

    /* Params: tuple: Tuple[...Args] */
    semantic_type_t tuple_inst = semantic_type_create_generic_instance(
        ctx->allocator,
        builtin_table_lookup(table, "Tuple")->type,
        NULL);
    /* Override the type_args with the Args pack */
    vec_init_t vi = {.auto_dispose = false};
    vec_t inst_args = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);
    vec_push(inst_args, args_param);
    tuple_inst->impl->generic_instance.type_args = inst_args;
    type_hash_ensure(tuple_inst);
    vec_push(ctx->all_types, tuple_inst);

    /* Function params: (tuple: Tuple[...Args]) — no index param, N is in type args */
    vec_init_t vi2 = {.auto_dispose = false};
    vec_t params = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi2);
    vec_push(params, tuple_inst);

    /* Return type: Args[N] — TYPE_PACK_INDEX placeholder, resolved during type checking */
    semantic_type_t ret_type = semantic_type_create_pack_index(
        ctx->allocator, "Args", 1, 0);
    type_hash_ensure(ret_type);
    vec_push(ctx->all_types, ret_type);

    semantic_type_t get_type = semantic_type_create_function(
        ctx->allocator, ret_type, params, false);
    type_hash_ensure(get_type);
    vec_push(ctx->all_types, get_type);
    builtin_table_register(table, "getTupleItem",
                           BUILTIN_FUNC, get_type, BUILTIN_DISPATCH_GET);
  }

  /* builtin func set[N: u64, ...Args](tuple: Tuple[...Args], value: Args[N]): void
     Sets the N-th element of a tuple. Return type is void.
     The value type Args[N] is resolved during type checking like get. */
  {
    /* Reuse generic params from get: N: u64 (index 0), ...Args (index 1) */
    semantic_type_t n_param_s = semantic_type_create_generic_param(
        ctx->allocator, "N", 0, ctx->builtin_u64, true);
    type_hash_ensure(n_param_s);
    vec_push(ctx->all_types, n_param_s);

    semantic_type_t args_param_s = semantic_type_create_generic_pack(
        ctx->allocator, "Args", 1);
    type_hash_ensure(args_param_s);
    vec_push(ctx->all_types, args_param_s);

    /* Params: (tuple: Tuple[...Args], value: Args[N]) */
    semantic_type_t tuple_inst_s = semantic_type_create_generic_instance(
        ctx->allocator,
        builtin_table_lookup(table, "Tuple")->type,
        NULL);
    vec_init_t vi_s1 = {.auto_dispose = false};
    vec_t inst_args_s = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi_s1);
    vec_push(inst_args_s, args_param_s);
    tuple_inst_s->impl->generic_instance.type_args = inst_args_s;
    type_hash_ensure(tuple_inst_s);
    vec_push(ctx->all_types, tuple_inst_s);

    /* value param: Args[N] — TYPE_PACK_INDEX placeholder */
    semantic_type_t value_type = semantic_type_create_pack_index(
        ctx->allocator, "Args", 1, 0);
    type_hash_ensure(value_type);
    vec_push(ctx->all_types, value_type);

    vec_init_t vi_s2 = {.auto_dispose = false};
    vec_t params_s = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi_s2);
    vec_push(params_s, tuple_inst_s);   /* tuple arg */
    vec_push(params_s, value_type);     /* value arg */

    semantic_type_t set_type = semantic_type_create_function(
        ctx->allocator, ctx->builtin_void, params_s, false);
    type_hash_ensure(set_type);
    vec_push(ctx->all_types, set_type);
    builtin_table_register(table, "setTupleItem",
                           BUILTIN_FUNC, set_type, BUILTIN_DISPATCH_SET);
  }
}
