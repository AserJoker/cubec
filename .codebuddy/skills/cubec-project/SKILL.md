---
name: cubec-project
description: >
  This skill provides comprehensive knowledge of the Cubec compiler project.
  It should be used when working on any code in the cubec codebase,
  including core data structures, lexer/parser modules, tests, or build system.
  Load this skill whenever the user asks about or modifies files in the cubec project.
---

# Cubec Compiler Project

## Overview

**Cubec** is a C-like programming language compiler frontend, written in C11. It implements a complete lexer (tokenizer) and a partially complete parser. The project uses a hand-crafted "C-style OOP" pattern with virtual tables (`type_t`) for lifecycle management, unified memory management via `allocator_t`, and built-in memory leak detection.

## Directory Structure

```
cubec/
├── CMakeLists.txt              # Build configuration
├── include/                    # Public headers
│   ├── core/                   # Core data structure library
│   │   ├── allocator.h         # Memory allocator
│   │   ├── error.h             # Error handling (TRY/THROW/CATCH macros)
│   │   ├── list.h              # Doubly-linked list
│   │   ├── location.h          # Source location info
│   │   ├── map.h               # Hash map (switches to red-black tree at >=16 entries)
│   │   ├── node.h              # AST node base class
│   │   ├── position.h          # Line/column position
│   │   ├── rbtree.h            # Red-black tree (uint64_t keys)
│   │   ├── string.h            # Dynamic string
│   │   ├── token.h             # Token base class
│   │   ├── type.h              # Type system (virtual table)
│   │   └── vec.h               # Dynamic array (vector)
│   └── cubec/                  # Language frontend module
│       ├── expression.h        # Expression AST node
│       ├── literal.h           # Literal AST node (abstract)
│       ├── literal_char.h      # Character literal
│       ├── literal_identifier.h# Identifier literal
│       ├── literal_numeric.h   # Numeric literal (with type suffixes)
│       ├── literal_string.h    # String literal
│       ├── node.h              # AST node kind enum (42 types)
│       ├── program.h           # Top-level program node
│       ├── statement_empty.h   # Empty statement node
│       └── token.h             # Token kind enum + lexer interface
├── src/                        # Source files (mirrors include/ structure)
│   ├── main.c                  # Entry point (placeholder)
│   ├── core/                   # Core data structure implementations
│   └── cubec/                  # Lexer + parser implementations
├── test/                       # Tests (~156 test cases, Google Test + C++20)
│   ├── main.cpp                # Test entry point
│   ├── common/test_common.h    # RAII test allocator helper
│   ├── core/                   # Tests for core data structures
│   └── cubec/                  # Tests for lexer/parser
└── demo/
    └── index.cubec             # Sample source file
```

## Core Architecture: type_t + allocator_t

### type_t (Virtual Table)

Every "class" has a global `type_t` instance describing its lifecycle:

```c
struct _type_t {
    const size_t size;           // Object size
    const char *name;            // Type name (for debug/leak reporting)
    type_init_fn_t init;         // Constructor
    type_dispose_fn_t dispose;   // Destructor
    type_clone_fn_t clone;       // Deep copy
    type_move_fn_t move;         // Move (transfer ownership)
};
```

Each module exports `extern type_t g_xxx_type` with registered lifecycle functions.

### allocator_t

- Wraps `malloc`/`free` (injectable custom allocator)
- All allocated memory tracked in a doubly-linked list of `alloc_chunk_t` (records size, type pointer, unique ID)
- **OOM policy**: Allocation failure → `abort()` crash. Callers who need graceful degradation should implement it in their custom `alloc_fn`, not by checking return values.
- **Key APIs:**
  - `allocator_alloc(allocator, size)` — Allocate raw zero-initialized memory; never returns NULL (aborts on OOM). Returns NULL only when `size == 0`.
  - `allocator_create(allocator, type, arg)` — Create typed object (calls `type->init`); never returns NULL (aborts on OOM)
  - `allocator_free(allocator, data)` — Free memory (auto-calls `type->dispose`), NULL-safe
  - `value_get_type(data)` / `value_get_id(data)` — Introspection
  - `value_clone(allocator, data)` / `value_move(allocator, data)` — Clone/move
- `create_allocator` aborts on OOM; `delete_allocator` is NULL-safe — reports all unfreed memory

### Inheritance Pattern

C struct nesting simulates single inheritance:

```
node_t (core/node.h)
  └── cubec_expression_t (cubec/expression.h)
        └── cubec_literal_t (cubec/literal.h)
              ├── cubec_literal_char_t
              ├── cubec_literal_identifier_t
              ├── cubec_literal_numeric_t
              └── cubec_literal_string_t
  └── cubec_statement_empty_t
  └── cubec_program_node_t
```

Subclasses embed the parent via a `super` field and call parent's `type->init` during initialization.

## Core Data Structures

### vec_t — Dynamic Array
- Like `std::vector<void*>`
- Operations: push, pop, get, set, insert, remove, resize
- Capacity starts at 0, first resize → 8, then doubles
- **Iterator operations**: `vec_iter_get` (O(1)), `vec_iter_set` (O(1)), `vec_iter_remove` (O(n) — shifts left, iterator stays at same index), `vec_iter_next` (O(1))
- **Iterator model**: `vec_iter_first` positions at index 0; `vec_iter_next` returns current data then advances; `vec_iter_get` reads without advancing; `vec_iter_remove` removes current element, subsequent elements shift left, iterator stays at same index (now pointing to the next element)
- Supports `auto_dispose` mode

### list_t — Doubly-Linked List
- Standard doubly-linked list with head/tail pointers
- **Indexed insert**: `list_insert(idx, data)` — O(n), traverse from nearer end
- **Iterator operations** (O(1)): `list_iter_get`/`list_iter_set`/`list_iter_remove` — direct node access, preferred API pattern
- Queue/stack: `push`/`pop` (tail), `unshift`/`shift` (head)
- **Iterator model**: `list_iter_first` positions at the first element; `list_iter_next` returns current data then advances; `list_iter_get` reads without advancing; `list_iter_remove` deletes current and advances to next
- No indexed read/write/delete — all random access must go through the iterator
- Supports clone/move, `auto_dispose` mode

### rbtree_t — Red-Black Tree
- Complete RB-tree with uint64_t keys
- Standard operations: insert, find, remove, clear
- Left/right rotation, insert fixup, delete fixup all implemented
- In-order traversal iterator `rbtree_iter_t`
- Supports auto_dispose

### map_t — Dictionary/Map
- Implemented as two `vec_t` (keys + values) + index
- Small scale (< 16 entries): hash table (16 buckets, chaining)
- Large scale (>= 16 entries): auto-converts to red-black tree index (irreversible)
- `map_remove`: uses iterator-based `remove_entry_from_bucket` (single-pass O(n) traversal) for hash bucket removal
- Uses `value_get_id(key)` as hash key
- Provides `map_iter_t`

### string_t — Dynamic String
- Like `std::string`, null-terminated
- Initial capacity 1, doubles on expansion
- Operations: get, set, concat, nconcat (fixed-length)
- Clone allocates new buffer matching source capacity
- Move transfers data pointer, leaves source with empty 8-byte buffer

### position_t / location_t
- `position_t`: line, column, offset pointer
- `location_t`: filename + begin/end positions
- `location_get`: extracts source text between begin/end offsets via `memcpy` + explicit `\0` termination
- `location_is`: compares location text to a string via `strncmp`

### token_t / node_t (Base Classes)
- `token_t`: allocator, kind (uint32_t), location
- `node_t`: allocator, kind (uint32_t), location, parent pointer

## Error Handling System

- Uses `thread_local` global error pointer `g_error`
- `error_t` contains: error message (1024 bytes, formatted via `vsnprintf`) + call stack (up to 64 frames)
- `error_push()`: NULL-safe — no-op if `g_error == NULL` (safeguard against calling before any error thrown)
- `error_to_string()`: formats error to readable string with call stack, uses `PRIuPTR` for portability
- Rust-style macros:
  - `THROW(ret, fmt, ...)` — Throw error and return
  - `TRY(ret, expr)` — Execute expr, propagate error on failure
  - `CATCH_ERROR(expr, onerror)` — Execute expr, run onerror on failure
  - `TRY_LOCAL` / `TRY_VOID_LOCAL` / `THROW_LOCAL` — Jump to `onerror:` label
- Uses GCC extensions: `__auto_type`, statement expressions `({...})` — requires `-std=gnu11` (CMake default)

## Lexer (src/cubec/token.c, ~675 lines)

### Token Types (9 kinds)
- `WHITESPACE`, `EOF`, `COMMENT` (`//`), `MULTILINE_COMMENT` (`/* */`)
- `IDENTIFIER`, `NUMERIC`, `SYMBOL`, `KEYWORD`, `STRING`, `CHAR`

### Key Functions
- `read_unicode` — UTF-8 decoder (1-4 bytes), returns Unicode codepoint
- `read_symbol_token` — Longest match, supports 50+ symbols (compound: `&&=`, `||=`, `>>`, `<<`, etc.)
- `read_whitespace_token` — Uses ICU `u_isWhitespace`
- `read_comment_token` — Single-line `//`
- `read_multiline_comment_token` — Multi-line `/* */` (no nesting)
- `read_numeric_token` — Decimal, hex `0x`, octal `0o`, binary `0b`, float, scientific notation `e/E`
- `read_string_token` — Escape sequences: `\n`, `\t`, `\r`, `\\`, `\'`, `\"`, `\0`, `\xHH`, `\u{...}`
- `read_char_token` — Character literal, same escape support
- `read_identifier_token` — Uses ICU `u_isIDStart`/`u_isIDPart`, also detects keywords
- `read_token` — Tries all token types in priority order
- `resolve_token_list` — Complete lexer entry point, returns token vector

### Keywords (29 total)
`break`, `case`, `comptime`, `const`, `continue`, `defer`, `do`, `else`, `enum`, `export`, `extern`, `for`, `foreach`, `func`, `if`, `import`, `in`, `inline`, `mutable`, `of`, `pub`, `register`, `return`, `struct`, `switch`, `test`, `union`, `volatile`, `while`

### Known Issue
Whitespace tokens are sometimes incorrectly marked as `SYMBOL` (documented as "bug" in tests).

## Parser (Partially Implemented)

### Implemented
- `read_atom` (expression.c) — Parses: string > numeric > identifier > char literals
- `read_literal_char` — Character literal AST node
- `read_literal_identifier` — Identifier AST node
- `read_literal_numeric` — Numeric AST node, auto-detects int/float, supports type suffixes (`i8`-`i64`, `u8`-`u64`, `f16`-`f64`)
- `read_literal_string` — String AST node, supports auto-concatenation of adjacent strings
- `read_statement_empty` — Empty statement (`;`)
- `read_program_node` — Top-level entry, loops parsing statements until EOF

### Not Yet Implemented (42 AST node kinds defined, most unimplemented)
Most statement types (if, for, while, switch, defer, etc.), expression types (binary, call, member, etc.), and all declaration types are defined as enums but lack parser implementations.

## Build System

| Setting | Value |
|---------|-------|
| CMake minimum | 3.12 |
| C standard | C11 (GNU extensions enabled via CMake default) |
| C++ standard | C++20 |
| Toolchain | vcpkg (conditional, only if `VCPKG_ROOT` env set) |
| Compile defines | MSVC: `_CRT_SECURE_NO_WARNINGS`; Linux/GCC: `_GNU_SOURCE` |
| External deps | ICU (i18n, uc, data, io), Google Test, Threads |
| Output dir | `${PROJECT_SOURCE_DIR}` (project root) |

### Cross-Platform Support
- Windows + MSVC/Clang: fully supported, Clang needs `clang_rt.builtins-x86_64`
- Linux + GCC: supported (requires glibc >= 2.28 for `<threads.h>`)
- `Threads::Threads` linked on all platforms (provides `-pthread` on Linux)

### Build Targets
1. **cubecc** — Compiler executable (links `src/core/`, `src/engine/`, `src/cubec/`, `src/reader/`, `src/writer/`, `src/c/` + `src/main.c`). Note: `engine/`, `reader/`, `writer/`, `c/` directories don't exist yet — future modules for semantic analysis, source reading, code generation, and C backend.
2. **cubec_test** — Test executable (links all source files + test/*.cpp, depends on GTest)

### Platform-Specific Linking
- Windows + Clang: extra link `clang_rt.builtins-x86_64`
- Linux + GCC: `-pthread` via `Threads::Threads`

## Testing

- Framework: Google Test + C++20
- Helper: `test_allocator` RAII class in `test/common/test_common.h`
- Total: ~156 test cases

### Core Tests
- `dt_allocator.cpp` (11 cases) — create/destroy, alloc/free, zero-size, NULL-free, multi-alloc, type create, value introspection, clone, move
- `dt_vec.cpp` (20 cases) — create/resize, push/get, pop, set, insert, remove, resize, get_data, initial capacity + 10 iterator cases (iteration, empty, get, set, remove, remove_first, remove_last, remove_exhausted, traverse_remove_all, single_element)
- `dt_list.cpp` (20 cases) — full list operations using iterator-based get/set/remove; no indexed access APIs
- `dt_rbtree.cpp` (14 cases) — insert/find, duplicates, remove, clear, iteration, auto_dispose, clone, move
- `dt_map.cpp` (16 cases) — hash threshold (15 entries), RB-tree conversion threshold (20 entries), auto_dispose, clone, move
- `dt_string.cpp` (15 cases) — all string operations including Unicode, nconcat, binary data
- `dt_token.cpp` (42 cases) — all token types, all 29 keywords, all numeric formats, all escape sequences, comments, symbols

### Cubec Tests
- `dt_literal_char.cpp` (5 cases)
- `dt_literal_identifier.cpp` (5 cases)
- `dt_literal_numeric.cpp` (12 cases)
- `dt_literal_string.cpp` (12 cases)

## Planned Language Features (inferred from AST node types)

Cubec plans to support: defer statements, foreach loops, test blocks, comptime evaluation, struct/union/enum declarations, func declarations, slice types, pointer types, spread operator, decorators, switch pattern matching, and more.

## Coding Conventions

- C11 for library code, C++20 for tests
- Hand-crafted OOP via `type_t` virtual tables and struct nesting
- All memory managed through `allocator_t` — never use raw `malloc`/`free`
- Every data structure has a corresponding `g_xxx_type` global
- Error handling via `TRY`/`THROW`/`CATCH_ERROR` macros (Rust-like `?` pattern)
- Subclass init calls parent's `type->init` via `super` field
- Tests follow pattern: create allocator → test operations → destroy allocator (auto leak check)
- **Iterator-first for `list_t`**: Always use `list_iter_get`/`list_iter_set`/`list_iter_remove` for O(1) element access. Indexed `list_insert` is the only remaining O(n) index-based operation — used only for insertion position specification
- **Iterator for `vec_t`**: `vec_iter_first` creates iterator at index 0; `vec_iter_next` returns data then advances; `vec_iter_get`/`vec_iter_set` provide O(1) read/write at current position; `vec_iter_remove` deletes current element (O(n) shift), iterator stays at same index pointing to next element
