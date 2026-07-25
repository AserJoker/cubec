#include "engine/builtin.h"
#include "engine/builtin_cast.h"
#include "engine/builtin_debug.h"
#include "engine/builtin_collection.h"
#include "engine/builtin_tuple.h"
#include "engine/builtin_union.h"
#include "engine/builtin_string.h"
#include "engine/builtin_panic.h"
#include "engine/builtin_typename.h"
#include "engine/builtin_slice.h"
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

void builtin_table_init_defaults(builtin_table_t table, struct context *ctx) {
  builtin_table_init_debug(table, ctx);
  builtin_table_init_collection(table, ctx);
  builtin_table_init_tuple(table, ctx);
  builtin_table_init_cast(table, ctx);
  builtin_table_init_union(table, ctx);
  builtin_table_init_string(table, ctx);
  builtin_table_init_panic(table, ctx);
  builtin_table_init_typename(table, ctx);
  builtin_table_init_slice(table, ctx);
}
