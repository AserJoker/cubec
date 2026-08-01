#include "engine/scope.h"
#include <string.h>

struct _scope_t {
  allocator_t allocator;
  scope_t parent;          /**< Parent scope (lexical) */
  vec_t symbols;           /**< vec of symbol* */
  enum scope_kind kind;    /**< Scope kind */
  location_t location;     /**< Where scope begins */
};

static void _scope_init(void *self, allocator_t allocator, void *arg) {
  (void)arg;
  scope_t s = (scope_t)self;
  s->allocator = allocator;
  s->parent = NULL;
  vec_init_t vec_init = {.auto_dispose = true};
  s->symbols = (vec_t)allocator_create(allocator, &g_vec_type, &vec_init);
  s->kind = SCOPE_GLOBAL;
}

static void _scope_dispose(void *self, allocator_t allocator) {
  scope_t s = (scope_t)self;
  allocator_free(allocator, &s->symbols);
}

type_t g_scope_type = {
    .size = sizeof(struct _scope_t),
    .name = "cubec.engine.scope",
    .init = (type_init_fn_t)_scope_init,
    .dispose = (type_dispose_fn_t)_scope_dispose,
};

scope_t scope_create(allocator_t allocator, scope_t parent,
                     enum scope_kind kind, location_t location) {
  scope_t s = (scope_t)allocator_create(allocator, &g_scope_type, NULL);
  if (!s) return NULL;
  s->parent = parent;
  s->kind = kind;
  s->location = location;
  return s;
}

scope_t scope_get_parent(scope_t self) { return self->parent; }

enum scope_kind scope_get_kind(scope_t self) { return self->kind; }

void scope_push_symbol(scope_t self, struct symbol *sym) {
  vec_push(self->symbols, sym);
}

struct symbol *scope_lookup_local(scope_t self, const char *name) {
  size_t size = vec_get_size(self->symbols);
  for (size_t i = 0; i < size; i++) {
    struct symbol *sym = (struct symbol *)vec_get(self->symbols, i);
    if (strcmp(sym->name, name) == 0) {
      return sym;
    }
  }
  return NULL;
}

vec_t scope_get_symbols(scope_t self) {
  return self ? self->symbols : NULL;
}

struct symbol *scope_lookup(scope_t self, const char *name) {
  /* For type scopes, only search type chain */
  if (self->kind == SCOPE_TYPE_INSTANCE || self->kind == SCOPE_TYPE_STATIC) {
    return scope_lookup_local(self, name);
  }

  /* Walk up lexical scope chain */
  for (scope_t s = self; s != NULL; s = s->parent) {
    if (s->kind == SCOPE_TYPE_INSTANCE || s->kind == SCOPE_TYPE_STATIC) {
      continue; /* skip type scopes in lexical lookup */
    }
    struct symbol *sym = scope_lookup_local(s, name);
    if (sym) {
      return sym;
    }
  }
  return NULL;
}

struct symbol *scope_lookup_instance(scope_t self, const char *name) {
  for (scope_t s = self; s != NULL; s = s->parent) {
    if (s->kind != SCOPE_TYPE_INSTANCE) {
      break;
    }
    struct symbol *sym = scope_lookup_local(s, name);
    if (sym) {
      return sym;
    }
  }
  return NULL;
}

struct symbol *scope_lookup_static(scope_t self, const char *name) {
  for (scope_t s = self; s != NULL; s = s->parent) {
    if (s->kind != SCOPE_TYPE_STATIC) {
      break;
    }
    struct symbol *sym = scope_lookup_local(s, name);
    if (sym) {
      return sym;
    }
  }
  return NULL;
}
