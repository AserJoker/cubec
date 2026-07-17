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
}
