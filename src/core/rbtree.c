#include "core/rbtree.h"
#include "core/allocator.h"
#include <stdbool.h>
#include <stdint.h>

#define IS_BLACK(node) ((node) == NULL || (node)->is_red == false)
#define IS_RED(node) ((node) != NULL && (node)->is_red == true)

typedef struct _rbtree_node_t rbtree_node_t;
struct _rbtree_node_t {
  rbtree_node_t *left;
  rbtree_node_t *right;
  rbtree_node_t *parent;
  uint64_t key;
  void *value;
  bool is_red;
};

struct _rbtree_t {
  allocator_t allocator;
  rbtree_node_t *root;
  size_t size;
  bool auto_dispose;
};

static void _rbtree_init(rbtree_t self, allocator_t allocator,
                         rbtree_init_t *init) {
  if (init) {
    self->auto_dispose = init->auto_dispose;
  } else {
    self->auto_dispose = false;
  }
  self->root = NULL;
  self->size = 0;
  self->allocator = allocator;
}

static void _rbtree_dispose(rbtree_t self, allocator_t allocator) {
  rbtree_clear(self);
}

static void clone_recursive(allocator_t allocator, rbtree_t self,
                            rbtree_node_t *node) {
  if (node == NULL) {
    return;
  }
  clone_recursive(allocator, self, node->left);
  void *cloned_value = value_clone(allocator, node->value);
  rbtree_insert(self, node->key, cloned_value);
  clone_recursive(allocator, self, node->right);
}

static void _rbtree_clone(rbtree_t self, allocator_t allocator,
                          rbtree_t another) {
  self->auto_dispose = another->auto_dispose;
  self->root = NULL;
  self->size = 0;
  self->allocator = allocator;
  if (another->root == NULL) {
    return;
  }
  clone_recursive(allocator, self, another->root);
}

static void _rbtree_move(rbtree_t self, allocator_t allocator,
                         rbtree_t another) {
  self->auto_dispose = another->auto_dispose;
  self->root = another->root;
  self->size = another->size;
  self->allocator = allocator;
  another->root = NULL;
  another->size = 0;
}

type_t g_rbtree_type = {
    .size = sizeof(struct _rbtree_t),
    .name = "cubec.core.rbtree",
    .init = (type_init_fn_t)_rbtree_init,
    .dispose = (type_dispose_fn_t)_rbtree_dispose,
    .clone = (type_clone_fn_t)_rbtree_clone,
    .move = (type_move_fn_t)_rbtree_move,
};

static rbtree_node_t *rbtree_find_node(rbtree_t self, uint64_t key);

static rbtree_node_t *create_rbtree_node(allocator_t allocator, uint64_t key,
                                         void *value) {
  rbtree_node_t *node = (rbtree_node_t *)allocator_alloc(
      allocator, sizeof(rbtree_node_t));
  node->left = NULL;
  node->right = NULL;
  node->parent = NULL;
  node->key = key;
  node->value = value;
  node->is_red = true;
  return node;
}

static void rotate_left(rbtree_t self, rbtree_node_t *x) {
  rbtree_node_t *y = x->right;
  x->right = y->left;
  if (y->left != NULL) {
    y->left->parent = x;
  }
  y->parent = x->parent;
  if (x->parent == NULL) {
    self->root = y;
  } else if (x == x->parent->left) {
    x->parent->left = y;
  } else {
    x->parent->right = y;
  }
  y->left = x;
  x->parent = y;
}

static void rotate_right(rbtree_t self, rbtree_node_t *x) {
  rbtree_node_t *y = x->left;
  x->left = y->right;
  if (y->right != NULL) {
    y->right->parent = x;
  }
  y->parent = x->parent;
  if (x->parent == NULL) {
    self->root = y;
  } else if (x == x->parent->right) {
    x->parent->right = y;
  } else {
    x->parent->left = y;
  }
  y->right = x;
  x->parent = y;
}

static void rbtree_insert_fixup(rbtree_t self, rbtree_node_t *z) {
  while (z->parent != NULL && IS_RED(z->parent)) {
    if (z->parent == z->parent->parent->left) {
      rbtree_node_t *y = z->parent->parent->right;
      if (IS_RED(y)) {
        z->parent->is_red = false;
        y->is_red = false;
        z->parent->parent->is_red = true;
        z = z->parent->parent;
      } else {
        if (z == z->parent->right) {
          z = z->parent;
          rotate_left(self, z);
        }
        z->parent->is_red = false;
        z->parent->parent->is_red = true;
        rotate_right(self, z->parent->parent);
      }
    } else {
      rbtree_node_t *y = z->parent->parent->left;
      if (IS_RED(y)) {
        z->parent->is_red = false;
        y->is_red = false;
        z->parent->parent->is_red = true;
        z = z->parent->parent;
      } else {
        if (z == z->parent->left) {
          z = z->parent;
          rotate_right(self, z);
        }
        z->parent->is_red = false;
        z->parent->parent->is_red = true;
        rotate_left(self, z->parent->parent);
      }
    }
  }
  self->root->is_red = false;
}

size_t rbtree_get_size(rbtree_t self) { return self->size; }

void *rbtree_insert(rbtree_t self, uint64_t key, void *value) {
  rbtree_node_t *y = NULL;
  rbtree_node_t *x = self->root;
  while (x != NULL) {
    y = x;
    if (key < x->key) {
      x = x->left;
    } else if (key > x->key) {
      x = x->right;
    } else {
      x->value = value;
      return x->value;
    }
  }
  rbtree_node_t *z = create_rbtree_node(self->allocator, key, value);
  z->parent = y;
  if (y == NULL) {
    self->root = z;
  } else if (key < y->key) {
    y->left = z;
  } else {
    y->right = z;
  }
  z->left = NULL;
  z->right = NULL;
  z->is_red = true;
  self->size++;
  rbtree_insert_fixup(self, z);
  return z->value;
}

static void rbtree_transplant(rbtree_t self, rbtree_node_t *u,
                              rbtree_node_t *v) {
  if (u->parent == NULL) {
    self->root = v;
  } else if (u == u->parent->left) {
    u->parent->left = v;
  } else {
    u->parent->right = v;
  }
  if (v != NULL) {
    v->parent = u->parent;
  }
}

static rbtree_node_t *rbtree_minimum(rbtree_node_t *node) {
  while (node->left != NULL) {
    node = node->left;
  }
  return node;
}

static void rbtree_delete_fixup(rbtree_t self, rbtree_node_t *x) {
  while (x != NULL && x != self->root && IS_BLACK(x)) {
    if (x == x->parent->left) {
      rbtree_node_t *w = x->parent->right;
      if (IS_RED(w)) {
        w->is_red = false;
        x->parent->is_red = true;
        rotate_left(self, x->parent);
        w = x->parent->right;
      }
      if (IS_BLACK(w->left) && IS_BLACK(w->right)) {
        w->is_red = true;
        x = x->parent;
      } else {
        if (IS_BLACK(w->right)) {
          w->left->is_red = false;
          w->is_red = true;
          rotate_right(self, w);
          w = x->parent->right;
        }
        w->is_red = x->parent->is_red;
        x->parent->is_red = false;
        w->right->is_red = false;
        rotate_left(self, x->parent);
        x = self->root;
      }
    } else {
      rbtree_node_t *w = x->parent->left;
      if (IS_RED(w)) {
        w->is_red = false;
        x->parent->is_red = true;
        rotate_right(self, x->parent);
        w = x->parent->left;
      }
      if (IS_BLACK(w->left) && IS_BLACK(w->right)) {
        w->is_red = true;
        x = x->parent;
      } else {
        if (IS_BLACK(w->left)) {
          w->right->is_red = false;
          w->is_red = true;
          rotate_left(self, w);
          w = x->parent->left;
        }
        w->is_red = x->parent->is_red;
        x->parent->is_red = false;
        w->left->is_red = false;
        rotate_right(self, x->parent);
        x = self->root;
      }
    }
  }
  if (x != NULL) {
    x->is_red = false;
  }
}

size_t rbtree_remove(rbtree_t self, uint64_t key) {
  rbtree_node_t *z = rbtree_find_node(self, key);
  if (z == NULL) {
    return self->size;
  }
  rbtree_node_t *y = z;
  rbtree_node_t *x;
  bool y_original_is_red = y->is_red;
  if (z->left == NULL) {
    x = z->right;
    rbtree_transplant(self, z, z->right);
  } else if (z->right == NULL) {
    x = z->left;
    rbtree_transplant(self, z, z->left);
  } else {
    y = rbtree_minimum(z->right);
    y_original_is_red = y->is_red;
    x = y->right;
    if (y->parent == z) {
      if (x != NULL) {
        x->parent = y;
      }
    } else {
      rbtree_transplant(self, y, y->right);
      y->right = z->right;
      y->right->parent = y;
    }
    rbtree_transplant(self, z, y);
    y->left = z->left;
    y->left->parent = y;
    y->is_red = z->is_red;
  }
  if (y_original_is_red == false) {
    rbtree_delete_fixup(self, x);
  }
  if (self->auto_dispose && z->value != NULL) {
    allocator_free(self->allocator, &z->value);
  }
  allocator_free(self->allocator, &z);
  self->size--;
  return self->size;
}

static rbtree_node_t *rbtree_find_node(rbtree_t self, uint64_t key) {
  rbtree_node_t *current = self->root;
  while (current != NULL) {
    if (key < current->key) {
      current = current->left;
    } else if (key > current->key) {
      current = current->right;
    } else {
      return current;
    }
  }
  return NULL;
}

void *rbtree_find(rbtree_t self, uint64_t key) {
  rbtree_node_t *node = rbtree_find_node(self, key);
  return node ? node->value : NULL;
}

static void clear_recursive(rbtree_t self, rbtree_node_t *node) {
  if (node == NULL) {
    return;
  }
  clear_recursive(self, node->left);
  clear_recursive(self, node->right);
  if (self->auto_dispose && node->value != NULL) {
    allocator_free(self->allocator, &node->value);
  }
  allocator_free(self->allocator, &node);
}

void rbtree_clear(rbtree_t self) {
  if (self->root == NULL) {
    return;
  }
  clear_recursive(self, self->root);
  self->root = NULL;
  self->size = 0;
}

static rbtree_node_t *rbtree_minimum_ancestor(rbtree_node_t *node) {
  while (node->parent != NULL) {
    if (node == node->parent->left) {
      return node->parent;
    }
    node = node->parent;
  }
  return NULL;
}

rbtree_iter_t rbtree_iter_first(rbtree_t tree) {
  rbtree_iter_t iter = {
      .tree = tree,
      .current = NULL,
  };
  if (tree->root == NULL) {
    return iter;
  }
  iter.current = tree->root;
  while (((rbtree_node_t *)iter.current)->left != NULL) {
    iter.current = ((rbtree_node_t *)iter.current)->left;
  }
  return iter;
}

void *rbtree_iter_next(rbtree_iter_t *iter) {
  if (iter->current == NULL) {
    return NULL;
  }
  rbtree_node_t *node = (rbtree_node_t *)iter->current;
  rbtree_node_t *result = node;
  if (node->right != NULL) {
    iter->current = node->right;
    while (((rbtree_node_t *)iter->current)->left != NULL) {
      iter->current = ((rbtree_node_t *)iter->current)->left;
    }
  } else {
    iter->current = rbtree_minimum_ancestor(node);
  }
  return result->value;
}
