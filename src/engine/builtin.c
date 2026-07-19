#include "engine/builtin.h"
#include "engine/builtin_dispatch.h"
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
    builtin_entry_t src = (builtin_entry_t)arg;
    entry->name = src->name;
    entry->type = src->type;
    entry->eval_call = src->eval_call;
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
                            semantic_type_t type, builtin_eval_call_fn eval_call) {
  struct builtin_entry init_data = {
      .name = name,
      .type = type,
      .eval_call = eval_call,
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
    builtin_table_register(table, "assert", assert_type, builtin_assert_eval);
  }

  /* builtin func length[T](list: T): u64 */
  {
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
    builtin_table_register(table, "length", length_type, builtin_length_eval);
  }

  /* builtin func get[N: u64, ...Args](tuple: <...Args>): Args[N] */
  {
    semantic_type_t n_param = semantic_type_create_generic_param(
        ctx->allocator, "N", 0, ctx->builtin_u64, true);
    type_hash_ensure(n_param);
    vec_push(ctx->all_types, n_param);

    semantic_type_t args_param = semantic_type_create_generic_pack(
        ctx->allocator, "Args", 1);
    type_hash_ensure(args_param);
    vec_push(ctx->all_types, args_param);

    /* Params: tuple: <...Args> — a TYPE_TUPLE with element types from the pack */
    vec_init_t evi = {.auto_dispose = false};
    vec_t elem_types = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &evi);
    vec_push(elem_types, args_param);
    semantic_type_t tuple_param_type = semantic_type_create_tuple(
        ctx->allocator, elem_types);
    type_hash_ensure(tuple_param_type);
    vec_push(ctx->all_types, tuple_param_type);

    vec_init_t vi2 = {.auto_dispose = false};
    vec_t params = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi2);
    vec_push(params, tuple_param_type);

    semantic_type_t ret_type = semantic_type_create_pack_index(
        ctx->allocator, "Args", 1, 0);
    type_hash_ensure(ret_type);
    vec_push(ctx->all_types, ret_type);

    semantic_type_t get_type = semantic_type_create_function(
        ctx->allocator, ret_type, params, false);
    type_hash_ensure(get_type);
    vec_push(ctx->all_types, get_type);
    builtin_table_register(table, "getTupleItem", get_type, builtin_get_eval);
  }

  /* builtin func set[N: u64, ...Args](tuple: <...Args>, value: Args[N]): void */
  {
    semantic_type_t n_param_s = semantic_type_create_generic_param(
        ctx->allocator, "N", 0, ctx->builtin_u64, true);
    type_hash_ensure(n_param_s);
    vec_push(ctx->all_types, n_param_s);

    semantic_type_t args_param_s = semantic_type_create_generic_pack(
        ctx->allocator, "Args", 1);
    type_hash_ensure(args_param_s);
    vec_push(ctx->all_types, args_param_s);

    /* Params: tuple: <...Args> — a TYPE_TUPLE with element types from the pack */
    vec_init_t evi_s = {.auto_dispose = false};
    vec_t elem_types_s = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &evi_s);
    vec_push(elem_types_s, args_param_s);
    semantic_type_t tuple_param_type_s = semantic_type_create_tuple(
        ctx->allocator, elem_types_s);
    type_hash_ensure(tuple_param_type_s);
    vec_push(ctx->all_types, tuple_param_type_s);

    semantic_type_t value_type = semantic_type_create_pack_index(
        ctx->allocator, "Args", 1, 0);
    type_hash_ensure(value_type);
    vec_push(ctx->all_types, value_type);

    vec_init_t vi_s2 = {.auto_dispose = false};
    vec_t params_s = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi_s2);
    vec_push(params_s, tuple_param_type_s);
    vec_push(params_s, value_type);

    semantic_type_t set_type = semantic_type_create_function(
        ctx->allocator, ctx->builtin_void, params_s, false);
    type_hash_ensure(set_type);
    vec_push(ctx->all_types, set_type);
    builtin_table_register(table, "setTupleItem", set_type, builtin_set_eval);
  }
}
