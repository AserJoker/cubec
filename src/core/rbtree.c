#include "core/rbtree.h"
#include "core/allocator.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct _rbtree_node_t {
  bool color;
  void *key;
  rbtree_node_t left;
  rbtree_node_t right;
  rbtree_node_t parent;
};
struct _rbtree_t {
  rbtree_node_t root;
  bool autofree;
  size_t size;
  size_t keysize;
  compare_fn_t compare;
  allocator_t allocator;
};

static void rbtree_dispose_node(rbtree_t self, rbtree_node_t node,
                                allocator_t allocator) {
  if (node) {
    rbtree_dispose_node(self, node->left, allocator);
    rbtree_dispose_node(self, node->right, allocator);
    if (self->autofree) {
      allocator_free(allocator, node->key);
    }
    allocator_free(allocator, node);
  }
}

static void rbtree_dispose(rbtree_t self, allocator_t allocator) {
  rbtree_dispose_node(self, self->root, allocator);
}

rbtree_t create_rbtree(allocator_t allocator, rbtree_initialize_t *initialize) {
  rbtree_t tree = allocator_alloc(allocator, sizeof(struct _rbtree_t),
                                  (dispose_fn_t)rbtree_dispose);
  tree->root = NULL;
  tree->autofree = false;
  tree->size = 0;
  tree->compare = NULL;
  if (initialize) {
    tree->autofree = initialize->autofree;
    tree->compare = initialize->compare;
  }
  tree->allocator = allocator;
  return tree;
}

static void rbtree_left_rotate(rbtree_t self, rbtree_node_t x) {
  /*
   *     p               p
   *     |               |
   *     x               y
   *    / \    =>       / \
   *   a   y           x   c
   *      / \         / \
   *     b   c       a   b
   */
  rbtree_node_t y = x->right;
  rbtree_node_t b = y->left;
  rbtree_node_t p = x->parent;

  x->right = b;
  if (b) {
    b->parent = x;
  }

  if (p == NULL) {
    self->root = y;
  } else if (p->left == x) {
    p->left = y;
  } else {
    p->right = y;
  }
  y->parent = p;

  y->left = x;
  x->parent = y;
}

static void rbtree_right_rotate(rbtree_t self, rbtree_node_t y) {
  /*
   *      p            p
   *      |            |
   *      y            x
   *     / \    =>    / \
   *    x   c        a   y
   *   / \              / \
   *  a   b            b   c
   */

  rbtree_node_t x = y->left;
  rbtree_node_t b = x->right;
  rbtree_node_t p = y->parent;

  y->left = b;
  if (b) {
    b->parent = y;
  }

  if (p == NULL) {
    self->root = x;
  } else if (p->left == y) {
    p->left = x;
  } else {
    p->right = x;
  }
  x->parent = p;

  x->right = y;
  y->parent = x;
}
static void rbtree_insert_fixup(rbtree_t self, rbtree_node_t node) {
  rbtree_node_t parent, gparent;
  while ((parent = node->parent) && parent && parent->color) {
    gparent = parent->parent;
    if (parent == gparent->left) {
      rbtree_node_t uncle = gparent->right;
      if (uncle && uncle->color) {
        uncle->color = false;
        parent->color = false;
        gparent->color = true;
        node = gparent;
        continue;
      }
      if (parent->right == node) {
        rbtree_left_rotate(self, parent);
        rbtree_node_t tmp = parent;
        parent = node;
        node = tmp;
      }
      parent->color = false;
      gparent->color = true;
      rbtree_right_rotate(self, gparent);
    } else {
      rbtree_node_t uncle = gparent->left;
      if (uncle && uncle->color) {
        uncle->color = false;
        parent->color = false;
        gparent->color = true;
        node = gparent;
        continue;
      }
      if (parent->left == node) {
        rbtree_right_rotate(self, parent);
        rbtree_node_t tmp = parent;
        parent = node;
        node = tmp;
      }
      parent->color = false;
      gparent->color = true;
      rbtree_left_rotate(self, gparent);
    }
  }
  self->root->color = false;
}

static void rbtree_node_dispose(allocator_t allocator, rbtree_node_t self) {}
static rbtree_node_t create_rbtree_node(allocator_t allocator) {
  rbtree_node_t node = allocator_alloc(allocator, sizeof(struct _rbtree_node_t),
                                       (dispose_fn_t)rbtree_node_dispose);
  node->color = false;
  node->key = 0;
  node->left = NULL;
  node->right = NULL;
  node->parent = NULL;
  return node;
}

void rbtree_put(rbtree_t self, void *key, void *cmp_arg) {
  rbtree_node_t y = NULL;
  rbtree_node_t x = self->root;
  while (x != NULL) {
    y = x;
    if (self->compare ? self->compare(key, x->key, cmp_arg) < 0
                      : key < x->key) {
      x = x->left;
    } else if (self->compare ? self->compare(key, x->key, cmp_arg) == 0
                             : key == x->key) {
      if (x->key != key && self->autofree) {
        allocator_free(self->allocator, x->key);
      }
      x->key = key;
      return;
    } else {
      x = x->right;
    }
  }
  rbtree_node_t node = create_rbtree_node(self->allocator);
  node->key = key;
  node->parent = y;
  if (y != NULL) {
    if (self->compare ? self->compare(node->key, y->key, cmp_arg) < 0
                      : node->key < y->key) {
      y->left = node;
    } else {
      y->right = node;
    }
  } else {
    self->root = node;
  }
  node->color = true;
  rbtree_insert_fixup(self, node);
  self->size++;
}
bool rbtree_has(rbtree_t self, const void *key, void *cmp_arg) {
  return rbtree_get(self, key, cmp_arg) != NULL;
}
void *rbtree_get(rbtree_t self, const void *key, void *cmp_arg) {
  rbtree_node_t y = NULL;
  rbtree_node_t x = self->root;
  while (x != NULL) {
    y = x;
    if (self->compare ? self->compare(key, x->key, cmp_arg) < 0
                      : key < x->key) {
      x = x->left;
    } else if (self->compare ? self->compare(key, x->key, cmp_arg) == 0
                             : key == x->key) {
      return x->key;
    } else {
      x = x->right;
    }
  }
  return NULL;
}

static void rbtree_remove_fixup(rbtree_t self, rbtree_node_t node) {
  rbtree_node_t parent = node->parent;
  rbtree_node_t sibling = NULL;
  if (node == parent->left) {
    sibling = parent->right;
    if (!sibling->color) {
      // parent->right[Black]
      if (sibling->right && sibling->right->color) {
        // RR S[Black] parent->right[Black]->right[Red]
        sibling->right->color = sibling->color;
        sibling->color = parent->color;
        parent->color = false;
        rbtree_left_rotate(self, parent);
      } else if (sibling->left && sibling->left->color) {
        // RL
        // parent->right[Black]->left[Red]
        // parent->right[Black]->right?[Black]
        sibling->left->color = parent->color;
        parent->color = false;
        rbtree_right_rotate(self, sibling);
        rbtree_left_rotate(self, parent);
      } else if ((!sibling->left || !sibling->left->color) &&
                 (!sibling->right || !sibling->right->color)) {
        // parent->right[Black]->left?[Black]
        // parent->right[Black]->right?[Black]
        sibling->color = true;
        if (parent->color) {
          parent->color = false;
        } else if (parent != self->root) {
          rbtree_remove_fixup(self, parent);
        }
      }
    } else {
      bool color = sibling->color;
      sibling->color = parent->color;
      parent->color = color;
      rbtree_left_rotate(self, parent);
      rbtree_remove_fixup(self, node);
    }
  } else {
    sibling = parent->left;
    if (!sibling->color) {
      if (sibling->right && sibling->right->color) { // LR
        sibling->right->color = parent->color;
        parent->color = false;
        rbtree_left_rotate(self, sibling);
        rbtree_right_rotate(self, parent);
      } else if (sibling->left && sibling->left->color) { // LL
        sibling->left->color = sibling->color;
        sibling->color = parent->color;
        parent->color = false;
        rbtree_right_rotate(self, parent);
      } else if ((!sibling->left || !sibling->left->color) &&
                 (!sibling->right || !sibling->right->color)) {
        sibling->color = true;
        if (parent->color) {
          parent->color = false;
        } else if (parent != self->root) {
          rbtree_remove_fixup(self, parent);
        }
      }
    } else {
      bool color = sibling->color;
      sibling->color = parent->color;
      parent->color = color;
      rbtree_right_rotate(self, parent);
      rbtree_remove_fixup(self, node);
    }
  }
}

void rbtree_remove(rbtree_t self, const void *key, void *cmp_arg) {
  rbtree_node_t node = self->root;
  while (node) {
    if (self->compare ? self->compare(key, node->key, cmp_arg) == 0
                      : key == node->key) {
      break;
    }
    if (self->compare ? self->compare(key, node->key, cmp_arg) < 0
                      : key < node->key) {
      node = node->left;
    } else {
      node = node->right;
    }
  }
  if (!node) {
    return;
  }
  if (node->left && node->right) {
    rbtree_node_t replace = node->right;
    while (replace->left) {
      replace = replace->left;
    }
    rbtree_node_t tmp = node;
    node->key = replace->key;
    replace->key = tmp->key;
    node = replace;
  }
  rbtree_node_t parent = node->parent;
  rbtree_node_t child = NULL;
  if (node->left) {
    child = node->left;
  } else if (node->right) {
    child = node->right;
  }
  if (child) {
    if (!parent) {
      self->root = child;
    } else if (node == parent->left) {
      parent->left = child;
    } else {
      parent->right = child;
    }
    child->parent = parent;
    child->color = false;
  } else if (!parent) {
    self->root = NULL;
  } else {
    if (node->color) {
      if (parent->left == node) {
        parent->left = NULL;
      } else {
        parent->right = NULL;
      }
    } else {
      rbtree_remove_fixup(self, node);
      if (node == parent->left) {
        parent->left = NULL;
      } else if (node == parent->right) {
        parent->right = NULL;
      }
    }
  }
  if (self->autofree) {
    allocator_free(self->allocator, node->key);
  }
  allocator_free(self->allocator, node);
  self->size--;
}
size_t rbtree_size(rbtree_t self) { return self->size; }
rbtree_node_t rbtree_get_first(rbtree_t self) {
  if (self->root) {
    rbtree_node_t node = self->root;
    while (node->left) {
      node = node->left;
    }
    return node;
  }
  return NULL;
}
rbtree_node_t rbtree_get_last(rbtree_t self) {
  if (self->root) {
    rbtree_node_t node = self->root;
    while (node->right) {
      node = node->right;
    }
    return node;
  }
  return NULL;
}
rbtree_node_t rbtree_node_next(rbtree_node_t self) {
  if (self->right) {
    rbtree_node_t node = self->right;
    while (node->left) {
      node = node->left;
    }
    return node;
  }
  if (self->parent) {
    if (self == self->parent->left) {
      return self->parent;
    }
    while (self->parent && self == self->parent->right) {
      self = self->parent;
    }
    return self->parent;
  }
  return NULL;
}
rbtree_node_t rbtree_node_last(rbtree_node_t self) {
  if (self->left) {
    rbtree_node_t node = self->left;
    while (node->right) {
      node = node->right;
    }
    return node;
  }
  if (self->parent) {
    if (self == self->parent->right) {
      return self->parent;
    }
    while (self->parent && self == self->parent->left) {
      self = self->parent;
    }
    return self->parent;
  }
  return NULL;
}
void *rbtree_node_get(rbtree_node_t self) { return self->key; }