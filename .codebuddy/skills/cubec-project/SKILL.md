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

**Cubec** is a C-like programming language compiler frontend, written in C11. It implements a complete lexer (tokenizer), parser, semantic analysis engine, and comptime compile-time evaluator. The project uses a hand-crafted "C-style OOP" pattern with virtual tables (`type_t`) for lifecycle management, unified memory management via `allocator_t`, and built-in memory leak detection.

## Directory Structure

```
cubec/
├── CMakeLists.txt              # Build configuration
├── include/                    # Public headers
│   ├── core/                   # Core data structure library
│   │   ├── allocator.h         # Memory allocator
│   │   ├── error.h             # Error handling (TRY/THROW/CATCH macros)
│   │   ├── icu_data.h          # ICU common data initialization
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
│       ├── declaration.h       # Declaration base class (abstract)
│       ├── declaration_array.h # Array declaration ([ <expr> ] <type>)
│       ├── declaration_pointer.h # Pointer declaration (* [const] [volatile] <type>)
│       ├── declaration_slice.h  # Slice declaration ([] [const] [volatile] <type>)
│       ├── declaration_variable.h # Variable declarator (<identifier> [: <type>] = <expression>)
│       ├── function_argument.h # Function parameter node (<identifier> [: <type>])
│       ├── function_capture.h  # Function capture node (<identifier>)
│       ├── statement.h         # Statement dispatcher (read_statement)
│       ├── statement_block.h   # Block statement node ({ <statements> })
│       ├── statement_declaration.h # Declaration statement ([export|extern|builtin|comptime] var <declarator> ;)
│       ├── statement_empty.h   # Empty statement node
│       ├── statement_expression.h # Expression statement node (<expression>;)
│       ├── statement_function.h # Function declaration ([export] [inline] func | [extern] func | [builtin] func | [comptime] func name[params](args) [: type] { body } | ;)
│       ├── statement_import.h   # Import statement node (import <name> [as <alias>] from "<path>";)
│       ├── statement_return.h   # Return statement (return [expr];)
│       ├── expression.h        # Expression AST node
│       ├── expression_assignment.h  # Assignment expression (a = b, a += b, etc.)
│       ├── expression_binary.h  # Binary/prefix-unary expression (left/right/opt)
│       ├── expression_call.h    # Function-call expression callee(args)
│       ├── expression_comma.h  # Comma expression (a, b, c) — right-associative
│       ├── expression_function.h  # Function expression (func [name] |captures| [generic](params): type { body } | ;)
│       ├── expression_generic_instantiation.h  # Generic instantiation expr[a,b]
│       ├── expression_group.h   # Grouped expression ( expr )
│       ├── expression_initialize_field.h # Initialize field (.field = value)
│       ├── expression_initialize_list.h  # Initialize list (.<type>{items} or .{items})
│       ├── expression_member.h  # Member-access expression (host.field, instance member only)
│       ├── expression_namespace_access.h  # Namespace access (host::field)
│       ├── expression_postfix_unary.h  # Postfix unary: value.* (deref), value.& (addr)
│       ├── expression_typeof.h    # Typeof expression (typeof(<expression>), compile-time type computation)
│       ├── expression_sizeof.h    # Sizeof expression (sizeof(<expression>), compile-time size computation)
│       ├── expression_alignof.h   # Alignof expression (alignof(<expression>), compile-time alignment computation)
│       ├── expression_slice.h   # Slice expression (host[start:length])
│       ├── expression_spread.h  # Spread expression (...expr)
│       ├── expression_ternary.h # Ternary/conditional expression (cond ? consequent : alternate)
│       ├── expression_type_qualifier.h # Type qualifier expression (const/volatile <type>, has is_const + is_volatile flags)
│       ├── expression_type_function.h # Function type expression (func(params) -> type)
│       ├── literal.h           # Literal AST node (abstract)
│       ├── literal_char.h      # Character literal
│       ├── literal_identifier.h# Identifier literal
│       ├── literal_numeric.h   # Numeric literal (with type suffixes)
│       ├── literal_string.h    # String literal
│       ├── literal_undefined.h # Undefined literal (TDZ initializer)
│       ├── node.h              # AST node kind enum (71 node kinds)
│       ├── program.h           # Top-level program node
│       ├── generic_param.h      # Generic parameter node (T, T extends U, N: u64, ...T)
│       ├── statement_block.h   # Block statement node ({ <statements> })
│       ├── statement_declaration_type.h # Type alias declaration node
│       ├── statement_empty.h   # Empty statement node
│       ├── statement_expression.h # Expression statement node (<expression>;)
│       ├── statement_function.h # Function declaration ([export] [inline] func | [extern] func | [builtin] func | [comptime] func name[params](args) [: type] { body } | ;)
│       ├── statement_import.h   # Import statement node (import <name> [as <alias>] from "<path>";)
│       ├── statement.h         # Statement dispatcher (read_statement)
│       └── token.h             # Token kind enum + lexer interface
├── src/                        # Source files (mirrors include/ structure)
│   ├── main.c                  # Entry point (stub - initializes ICU + allocator)
│   ├── icu_data.c              # ICU common data (generated at build time)
│   ├── core/                   # Core data structure implementations
│   ├── cubec/                  # Lexer + parser implementations
│   └── engine/                 # Semantic analysis + comptime evaluator
│       ├── checker.c           # Checker lifecycle (create/dispose + pass orchestration)
│       ├── checker_collect.c   # Pass 1: symbol collection
│       ├── checker_check_stmt.c # Pass 2: type checking
│       ├── checker_evaluate.c  # Pass 2: comptime evaluation + type resolution
│       ├── comptime_eval.c     # Evaluator lifecycle (create/dispose)
│       ├── comptime_eval_expr.c # Expression evaluation
│       ├── comptime_eval_stmt.c # Statement execution
│       └── comptime_alloc.c    # Virtual memory (comptime allocator)
├── test/                       # Tests (1192 test cases, Google Test + C++20)
│   ├── main.cpp                # Test entry point
│   ├── common/test_common.h    # RAII test allocator helper
│   ├── core/                   # Tests for core data structures
│   ├── cubec/                  # Tests for lexer/parser
│   └── engine/                 # Tests for semantic analysis + comptime evaluator
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
  - `allocator_free(allocator, &data)` — Free memory (auto-calls `type->dispose`) and **set pointer to NULL**. NULL-safe on both the wrapper and the pointed-to pointer. Implemented as a macro wrapping `_allocator_free_impl(self, (void **)(ptr))` to handle C's `T**` → `void**` type incompatibility. Pass the **address** of your pointer: `allocator_free(a, &ptr)`.
  - `value_get_type(data)` / `value_get_id(data)` — Introspection
  - `value_clone(allocator, data)` / `value_move(allocator, data)` — Clone/move
- `create_allocator` aborts on OOM; `delete_allocator` is NULL-safe — reports all unfreed memory

### Inheritance Pattern

C struct nesting simulates single inheritance:

```
node_t (core/node.h)
  └── cubec_expression_t (cubec/expression.h)
        ├── cubec_declaration_t (cubec/declaration.h)  # Declaration base class (abstract)
        │     ├── cubec_declaration_array_t (cubec/declaration_array.h)  # [ <expr> ] <type>
        │     ├── cubec_declaration_pointer_t (cubec/declaration_pointer.h)  # * [const] [volatile] <type>
        │     └── cubec_declaration_slice_t (cubec/declaration_slice.h)  # [] [const] [volatile] <type>
        ├── cubec_expression_binary_t (cubec/expression_binary.h)
        ├── cubec_expression_call_t (cubec/expression_call.h)
        ├── cubec_expression_function_t (cubec/expression_function.h)  # func [name] |captures| [generic](params): type { body } | ;
        ├── cubec_expression_generic_instantiation_t (cubec/expression_generic_instantiation.h)
        ├── cubec_expression_group_t (cubec/expression_group.h)
        ├── cubec_expression_initialize_field_t (cubec/expression_initialize_field.h)  # .field = value
        ├── cubec_expression_initialize_list_t (cubec/expression_initialize_list.h)  # .<type>{items}
        ├── cubec_expression_member_t (cubec/expression_member.h)
        ├── cubec_expression_namespace_access_t (cubec/expression_namespace_access.h)  # host::field
        ├── cubec_expression_postfix_unary_t (cubec/expression_postfix_unary.h)  # value.*, value.&
        ├── cubec_expression_slice_t (cubec/expression_slice.h)
        ├── cubec_expression_spread_t (cubec/expression_spread.h)
        ├── cubec_expression_type_qualifier_t (cubec/expression_type_qualifier.h)  # const/volatile <type> (is_const + is_volatile flags)
        ├── cubec_expression_type_function_t (cubec/expression_type_function.h)  # func(params) -> type
        ├── cubec_expression_typeof_t (cubec/expression_typeof.h)  # typeof(<expression>)
        ├── cubec_expression_sizeof_t (cubec/expression_sizeof.h)  # sizeof(<expression>)
        ├── cubec_expression_alignof_t (cubec/expression_alignof.h)  # alignof(<expression>)
        └── cubec_literal_t (cubec/literal.h)
              ├── cubec_literal_char_t
              ├── cubec_literal_identifier_t
              ├── cubec_literal_numeric_t
              ├── cubec_literal_string_t
              └── cubec_literal_undefined_t
  └── cubec_function_argument_t (cubec/function_argument.h)  # func param (identifier [: type])
  └── cubec_function_capture_t (cubec/function_capture.h)  # func capture (identifier)
  └── cubec_statement_block_t
  └── cubec_statement_empty_t
  └── cubec_statement_expression_t
  └── cubec_statement_function_t (cubec/statement_function.h)  # func declaration
  └── cubec_statement_return_t (cubec/statement_return.h)  # return [expr];
  └── cubec_program_node_t
```

Subclasses embed the parent via a `super` field and call parent's `type->init` during initialization.

```
node_t (core/node.h)
  └── cubec_expression_t (cubec/expression.h)
        ├── cubec_declaration_t (cubec/declaration.h)  # Declaration base class (abstract)
        │     ├── cubec_declaration_array_t (cubec/declaration_array.h)  # [ <expr> ] <type>
        │     ├── cubec_declaration_pointer_t (cubec/declaration_pointer.h)  # * [const] [volatile] <type>
        │     └── cubec_declaration_slice_t (cubec/declaration_slice.h)  # [] [const] [volatile] <type>
        ├── cubec_expression_assignment_t (cubec/expression_assignment.h)
        ├── cubec_expression_binary_t (cubec/expression_binary.h)
        ├── cubec_expression_call_t (cubec/expression_call.h)
        ├── cubec_expression_comma_t (cubec/expression_comma.h)
        ├── cubec_expression_function_t (cubec/expression_function.h)  # func [name] |captures| [generic](params): type { body } | ;
        ├── cubec_expression_generic_instantiation_t (cubec/expression_generic_instantiation.h)
        ├── cubec_expression_group_t (cubec/expression_group.h)
        ├── cubec_expression_initialize_field_t (cubec/expression_initialize_field.h)  # .field = value
        ├── cubec_expression_initialize_list_t (cubec/expression_initialize_list.h)  # .<type>{items}
        ├── cubec_expression_member_t (cubec/expression_member.h)
        ├── cubec_expression_namespace_access_t (cubec/expression_namespace_access.h)  # host::field
        ├── cubec_expression_postfix_unary_t (cubec/expression_postfix_unary.h)  # value.*, value.&
        ├── cubec_expression_slice_t (cubec/expression_slice.h)
        ├── cubec_expression_spread_t (cubec/expression_spread.h)
        ├── cubec_expression_ternary_t (cubec/expression_ternary.h)
        ├── cubec_expression_type_qualifier_t (cubec/expression_type_qualifier.h)  # const/volatile <type> (is_const + is_volatile flags)
        ├── cubec_expression_type_function_t (cubec/expression_type_function.h)  # func(params) -> type
        ├── cubec_expression_typeof_t (cubec/expression_typeof.h)  # typeof(<expression>)
        ├── cubec_expression_sizeof_t (cubec/expression_sizeof.h)  # sizeof(<expression>)
        ├── cubec_expression_alignof_t (cubec/expression_alignof.h)  # alignof(<expression>)
        └── cubec_literal_t (cubec/literal.h)
              ├── cubec_literal_char_t
              ├── cubec_literal_identifier_t
              ├── cubec_literal_numeric_t
              ├── cubec_literal_string_t
              └── cubec_literal_undefined_t
  └── cubec_function_argument_t (cubec/function_argument.h)  # func param (identifier [: type])
  └── cubec_function_capture_t (cubec/function_capture.h)  # func capture (identifier)
  └── cubec_statement_empty_t
  └── cubec_statement_expression_t
  └── cubec_statement_function_t (cubec/statement_function.h)  # func declaration
  └── cubec_statement_return_t (cubec/statement_return.h)  # return [expr];
  └── cubec_program_node_t
```

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
- `location_is`: compares location text to a string via `strncmp` **plus length check** (`str[length] == '\0'`) to prevent prefix-only matches (e.g., single-char `b` would previously match keyword `break`)

### token_t / node_t (Base Classes)
- `token_t`: allocator, kind (uint32_t), location
- `node_t`: allocator, kind (uint32_t), location, parent pointer

## Error Handling System

- Uses `thread_local` global error pointer `g_error`
- `err_t` contains: error message (1024 bytes, formatted via `vsnprintf`) + call stack (up to 64 frames)
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

Note: `...` (ellipsis/spread) is tokenized as a `SYMBOL` with text `"..."`, relying on the symbols table's longest-match ordering (placed among the 3-character symbols: `&&=`, `||=`, `...`).

### Key Functions
- `read_unicode` — UTF-8 decoder (1-4 bytes), returns Unicode codepoint
- `read_symbol_token` — Longest match ordered by token length: 3-char (`&&=`, `||=`, `...`), 2-char (`==`, `!=`, `>>`, `<<`, `+=`, ...), 1-char (`=`, `!`, `+`, ...). The `...` operator is placed among the 3-character symbols so it matches before the single `.`.
- `read_whitespace_token` — Uses ICU `u_isWhitespace`
- `read_comment_token` — Single-line `//`
- `read_multiline_comment_token` — Multi-line `/* */` (no nesting)
- `read_numeric_token` — Decimal, hex `0x`, octal `0o`, binary `0b`, float, scientific notation `e/E`
- `read_string_token` — Escape sequences: `\n`, `\t`, `\r`, `\\`, `\'`, `\"`, `\0`, `\xHH`, `\u{...}`
- `read_char_token` — Character literal, same escape support
- `read_identifier_token` — Uses ICU `u_isIDStart`/`u_isIDPart`, also detects keywords
- `read_token` — Tries all token types in priority order
- `resolve_token_list` — Complete lexer entry point, returns token vector

### Keywords (40 total)
`as`, `alignof`, `break`, `builtin`, `case`, `comptime`, `const`, `continue`, `defer`, `do`, `else`, `enum`, `export`, `extends`, `extern`, `for`, `foreach`, `from`, `func`, `if`, `import`, `in`, `inline`, `interface`, `is`, `mutable`, `of`, `pub`, `register`, `return`, `sizeof`, `struct`, `switch`, `test`, `type`, `typeof`, `union`, `var`, `volatile`, `while`

### Known Issue
Whitespace tokens are sometimes incorrectly marked as `SYMBOL` (documented as "bug" in tests).

## Complete Token Pipeline

```
源码文件 (const char *source)
  │
  ▼                         阶段 1: 词法分析
resolve_token_list()  ──────────────────►  vec_t tokens
  │  (src/cubec/token.c)                    (token 仅存位置指针 offset,
  │  逐字符分类:                              不复制文本)
  │  ├─ EOF / WHITESPACE / COMMENT / MULTILINE_COMMENT
  │  ├─ IDENTIFIER (UCD: u_isIDStart/u_isIDPart)
  │  ├─ KEYWORD (40个: break, case, comptime, const, ...)
  │  ├─ NUMERIC (十进制/十六进制/八进制/二进制/浮点/科学计数法)
  │  ├─ SYMBOL (最长匹配: 3字符 > 2字符 > 1字符)
  │  ├─ STRING (支持转义 \n \t \xHH \u{...})
  │  └─ CHAR (支持转义, 同 STRING)
  │
  ▼                         阶段 2: 语法分析
read_program_node()   ──────────────────►  AST (cubec_program_node_t)
  │  (src/cubec/program.c)
  │  skip_whitespace → 循环 read_statement
  │
  └── 表达式解析子流程 (Precedence Climbing):
      read_expression = read_expression_type
        └── read_expression_ternary()
              ├── condition: read_expression_binary()
              │     ├── read_unary()              ← 前缀一元: ! + - ~
              │     │     ├── read_expression_prefix()  → 递归 read_unary
              │     │     └── read_value()              →
              │     │           ├── read_atom()          基础值
              │     │           │   ├─ read_expression_initialize_list() .<type>{items}
              │     │           │   ├─ read_expression_typeof()    typeof(expr) 编译期类型计算
              │     │           │   ├─ read_expression_sizeof()    sizeof(expr) 编译期大小计算
              │     │           │   ├─ read_expression_alignof()   alignof(expr) 编译期对齐计算
              │     │           │   ├─ read_expression_type_function()  func(i32) -> type
              │     │           │   ├─ read_expression_function()  func |caps| (params): type { body }
              │     │           │   ├─ read_expression_group()    (...)
              │     │           │   ├─ read_expression_type_qualifier()  const/volatile <type>
              │     │           │   ├─ read_declaration_pointer()       * [const] [volatile] <type>
              │     │           │   ├─ read_declaration_slice()         [] [const] [volatile] <type>
              │     │           │   ├─ read_declaration_array()         [expr] [const] [volatile] <type>
              │     │           │   ├─ read_literal_string()      "..."
              │     │           │   ├─ read_literal_numeric()     42, 0xFF, 3.14e5
              │     │           │   ├─ read_literal_undefined()   undefined
              │     │           │   ├─ read_literal_identifier()  foo
              │     │           │   └─ read_literal_char()        'a'
              │     │           │
              │     │           └── Postfix 链循环 ──────
              │     │               ├─ read_expression_call()                    callee(args)
              │     │               ├─ read_expression_generic_instantiation()   callee[a,b]
              │     │               ├─ read_expression_postfix_unary()           value.* (解引用), value.& (取地址), value.? (try/unwrap)
              │     │               ├─ read_expression_member()                  host.field (实例成员访问)
              │     │               ├─ read_expression_namespace_access()         host::field (类型成员访问/命名空间导航)
              │     │               └─ (spread 不入 postfix 链, 由调用方显式调用)
              │     │
              │     └── read_binary_rhs()         ← 中缀二元, 10级优先级
              │           1: ||    2: &&     3: |     4: ^     5: &
              │           6: == != extends  7: < > <= >=  8: << >> 9: + -  10: * / %
              │
              ├── consequent: read_expression()    ← 递归
              └── alternate: read_expression()     ← 递归
```

### Parsing Pipeline

Expression parsing follows a precedence-climbing architecture:

```
read_expression                       # Entry point (currently delegates to binary)
  └── read_expression_binary          # Binary precedence climbing
        ├── read_unary (static helper) # Prefix unary chain OR value
        │     ├── read_expression_prefix → recursive read_unary
        │     └── read_value → read_atom → postfix loop
        └── read_binary_rhs (static)   # RHS: precedence-climbing recursion

read_atom
  ├── read_expression_initialize_list # .<type>{items} or .{items}
  ├── read_expression_typeof         # typeof(<expression>)
  ├── read_expression_sizeof         # sizeof(<expression>)
  ├── read_expression_alignof        # alignof(<expression>)
  ├── read_expression_type_function   # func(i32) -> type (function type, no named params)
  ├── read_expression_function        # func|caps|(params): type { body } (function expression)
  ├── read_expression_group           # ( expr )
  ├── read_expression_type_qualifier  # const/volatile <type>
  ├── read_declaration_pointer        # * [const] [volatile] <type>
  ├── read_declaration_slice          # [] [const] [volatile] <type>
  ├── read_declaration_array          # [ <expr> ] <type>
  ├── read_literal_string
  ├── read_literal_numeric
  ├── read_literal_undefined
  ├── read_literal_identifier
  └── read_literal_char

# Standalone expression parsers (not part of main precedence chain):
read_expression_assignment            # Assignment (=, +=, -=, etc.)
read_expression_comma                 # Comma (a, b, c) — right-associative
read_expression_ternary               # Ternary (a ? b : c)
read_expression_spread                # Spread (...expr)

# Postfix operators (called from read_value loop):
read_expression_call                  # callee(args)
read_expression_slice                 # host[start:length] — MUST be before generic (uses lookahead for ':')
read_expression_generic_instantiation # callee[args]  — tried after slice to handle non-slice brackets
read_expression_postfix_unary         # value.* (deref), value.& (addr), value.? (try/unwrap) — MUST be before member (uses '.' token)
read_expression_member                # host.field (实例成员访问，. 仅用于对象/变量)
read_expression_namespace_access      # host::field (类型成员访问/命名空间导航，:: 用于类型级，. 用于实例级)
```

- **read_expression** → delegates to `read_expression_ternary` (identical to `read_expression_type`)
- **read_expression_ternary** → calls `read_expression_binary` for condition, then parses `? consequent : alternate`
- **read_expression_binary** → calls `read_unary` for LHS, then enters `read_binary_rhs` precedence climbing loop
- **read_unary** → tries `read_expression_prefix` first; if that returns NULL, falls back to `read_value`
- Design rule: all `read_xxx` entry points assume first token is ready for parsing (caller skips whitespace/comments before invoking)

### Implemented Modules

- `read_expression_assignment` (expression_assignment.c) — Parses assignment expressions: simple assignment (`a = b`) and compound assignment (`+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`, `^=`, `<<=`, `>>=`). Left operand must be an lvalue (identifier, member access, dereference, or subscript). Right operand parsed via `read_expression_ternary`. Returns `cubec_expression_assignment_t` wrapping the target and value. When no assignment operator is found, returns NULL gracefully.
- `read_expression_comma` (expression_comma.c) — Parses comma expressions (`a, b, c`). Right-associative: `a, b, c` parses as `comma(a, comma(b, c))`. Left operand tries `read_expression_assignment` first, then falls back to `read_expression_ternary`. After consuming a comma, recursively calls itself for the right operand; if that returns NULL, falls back to `read_expression_ternary`. When no comma is found after the left operand, returns the left operand directly (passthrough behavior).
- `read_expression_binary` (expression_binary.c) — Full precedence-climbing binary expression parser. Supports 10 precedence levels: `\|\|` < `&&` < `\|` < `^` < `&` < `== != extends` < `< > <= >=` < `<< >>` < `+ -` < `* / %`. The `extends` keyword is a binary operator at the same precedence level as `==` and `!=` (level 6), handled separately in `get_binary_precedence()` via keyword check. Note: assignment (`=`, `+=`, etc.) and comma (`,`) are NOT part of this binary precedence table — they are parsed as separate expression types. Calls `read_unary` for operands, uses `read_binary_rhs` for right-recursive precedence climbing.
- `read_expression_prefix` (expression_binary.c) — Parses prefix unary operators (`!`, `+`, `-`, `~`); right operand parsed via recursive `read_unary` (NOT `read_expression`, which prevents incorrect binding like `-42 * 3` being parsed as `-(42*3)`); supports chained `!!x`, `--n`; returns `cubec_expression_binary_t` with `left=NULL`; **does NOT call `skip_whitespace` at entry**
- `read_expression_postfix_unary` (expression_postfix_unary.c) — Parses postfix unary operators `value.*` (dereference), `value.&` (address-of), and `value.?` (try/unwrap). Composed of separate `.` and `&`/`*`/`?` tokens combined by the parser. Uses lookahead for `:` to distinguish from slice expressions. Must be called before `read_expression_member` since both use the `.` token. Returns `cubec_expression_postfix_unary_t` with `opt` set to `".*"`, `".&"`, or `".?"`.
- `read_value` (expression.c) — Atom + recursive postfix loop. Calls `read_atom` first, then in while loop: calls `skip_whitespace`, then tries postfix operators in order: `read_expression_call` for `callee(args)`, `read_expression_slice` for `host[start:length]`, `read_expression_generic_instantiation` for `callee[args]`, `read_expression_postfix_unary` for `value.*`, `value.&`, and `value.?`, then `read_expression_member` for `.field` access. Slice is tried before generic instantiation using lookahead for `:` to distinguish `arr[0:10]` (slice) from `arr[0]` (generic). Postfix unary is tried before member since both start with `.`.
- `read_expression_call` (expression_call.c) — Parses C-style function call `callee(arg1, arg2, ...)`. Called from `read_value` as a postfix operator with `callee` already parsed. Each argument first tries `read_expression_spread` (supporting `...expr` including pack expansion `...args`), then falls back to `read_expression`. Returns NULL if next token is not `(`; THROW errors on malformed arguments (trailing comma, unclosed paren). Supports chained calls `foo()()` and mix with member: `obj.method()`, `foo().field`. **Ownership**: `arguments` vec created with `auto_dispose=true` in parser; `init` directly takes the pointer (no copy), ownership fully transferred to node.
- `read_expression_generic_instantiation` (expression_generic_instantiation.c) — Parses generic instantiation `callee[arg1, arg2, ...]`. Uses `[` and `]` as delimiters. Arguments parsed via `read_expression`, each first tries `read_expression_spread` (supporting `...expr` in generic args). Returns NULL if next token is not `[`; THROW errors on malformed arguments (trailing comma, unclosed bracket). Supports chained instantiations `fn[a][b]` and mixing with calls and member access: `fn[a]()`, `fn[a].field`, `foo()[a]`. Single-arg form `obj[0]` is syntactically ambiguous with member access — disambiguation deferred to semantic analysis.
- `read_expression_slice` (expression_slice.c) — Parses slice expression `host[start:length]`. Called from `read_value` as a postfix operator with `host` already parsed. Format: `host[start:length]` where `start` and `length` are both optional (at least `:` must be present). If `[` doesn't follow, returns NULL gracefully. If `[]` (empty brackets), throws error. Uses `read_expression` for parsing start/length expressions. Supports chained slices `arr[1:2][0:1]` and mixing with calls and member access: `arr[0:1].field`, `getArr()[1:]`. Node fields: `host`, `start`, `length` (start/length may be NULL if omitted).
- `read_atom` (expression.c) — Parses in order: `read_expression_initialize_list` → `read_expression_typeof` → `read_expression_sizeof` → `read_expression_alignof` → `read_expression_type_function` → `read_expression_function` → `read_expression_group` → `read_expression_type_qualifier` → `read_declaration_pointer` → `read_declaration_slice` → `read_declaration_array` → `read_literal_string` → `read_literal_numeric` → `read_literal_identifier` → `read_literal_char`. Type and value expressions share a unified parsing path: `read_expression_type` delegates directly to `read_expression`. Composite types (pointer/slice/array/qualifier/function type) greedily consume their inner type expression, including ternary. Use grouping `()` to prevent greedy consumption.
- `read_expression_group` (expression_group.c) — Parses parenthesized expression `( expr )`. Returns `cubec_expression_group_t` wrapping the inner expression. Tried first in `read_atom` so `(a + b)` is parsed as a group wrapping a binary expression.
- `read_expression_initialize_list` (expression_initialize_list.c) — Parses initialize list expression `.<type>{<items>}` or `.{<items>}`. Called from `read_atom` as a primary expression (tried before `read_expression_group`). Checks for `.` at current position, then looks ahead: `.` + `{` → anonymous (type=NULL), `.` + type expression + `{` → typed. Type is parsed via `read_expression_type` (supports member access, generic instantiation, pointer, etc.). Items are comma-separated and must be homogeneous: either all `initialize_field` (`.name = value`) or all positional expressions — mixing is an error. First item determines mode: tries `read_expression_initialize_field` first; if that fails, falls back to `read_expression`. In field mode, non-field items cause error; in positional mode, field-like items cause error. Disambiguation: `.{.Test{}}` is positional (`.Test{}` is a nested initialize_list expression), `.{.Test=123}` is field mode (`.Test=123` has `=`). THROW errors on: trailing comma, unclosed `}`, mixed field/positional items. Returns `cubec_expression_initialize_list_t` with `type` (nullable node_t), `items` (vec_t with auto_dispose=true), `is_field` (bool). Supports postfix chaining: `.Vec{1,2}.field`, and binary context: `1 + .Vec{1,2}`.
- `read_expression_initialize_field` (expression_initialize_field.c) — Parses initialize field expression `.identifier = expression`. Used inside `read_expression_initialize_list` to parse individual field items. Checks for `.` followed by identifier followed by `=`. Returns NULL if the `.` + identifier + `=` pattern is not matched (not an error — allows caller to try positional expression parsing). Returns `cubec_expression_initialize_field_t` with `field` (cubec_literal_identifier_t) and `value` (node_t).
- `read_generic_params` (generic_param.c) — Parses generic parameter list `[param1, param2, ...]`. Supports four forms: simple (`T`), constrained (`T extends Numeric`), value generic (`N: u64`), and rest param (`...Args`). Rest param is detected by checking for `...` symbol before reading identifier; if detected, `is_rest` is set to `true`. Constraint and value types are parsed via `read_expression_type` (greedy — consumes ternary). Parameters are comma-separated within `[]`. Pack params must be last; only one pack param allowed. Returns a vec of `cubec_generic_param_t`. Ownership: params vec is created with `auto_dispose=true`; caller takes ownership.
- `read_expression_spread` (expression_spread.c) — Parses spread operator `...<expr>`. Returns `cubec_expression_spread_t` wrapping the spread value. **Standalone function** — NOT called from `read_atom`/`read_value`/`read_expression`. Designed to be explicitly invoked by callers that support spread syntax (e.g., function arguments, struct initializers). Uses `read_expression` for the value so `...a + b` spreads the entire binary expression `a + b`.
- `read_expression_ternary` (expression_ternary.c) — Parses ternary/conditional expression `condition ? consequent : alternate`. Uses precedence climbing via `read_expression_binary` for the condition. Falls back gracefully if `?` is not found (returns condition as-is). Recursively calls `read_expression` for consequent and alternate to handle nested ternaries naturally. Full lifecycle: init/dispose/clone/move. Node fields: `condition`, `consequent`, `alternate`.
- `read_literal_char` — Character literal AST node
- `read_literal_undefined` — Undefined literal AST node (`undefined`), used as TDZ initializer in var declarations
- `read_literal_identifier` — Identifier AST node
- `read_literal_numeric` — Numeric AST node, auto-detects int/float, supports type suffixes (`i8`-`i64`, `u8`-`u64`, `f16`-`f64`)
- `read_literal_string` — String AST node, supports auto-concatenation of adjacent strings
- `read_statement_empty` — Empty statement (`;`)
- `read_statement_block` (statement_block.c) — Block statement (`{ <statements> }`). Parses a sequence of statements enclosed in curly braces, creating a new scope. The block may be empty (`{}`). Returns `cubec_statement_block_t` with a `statements` vec field. Returns NULL if current token is not `{`. THROW errors on unclosed brace or unexpected token in block.
- `read_statement_expression` (statement_expression.c) — Expression statement (`<expression>;`). Parses an expression via `read_expression`, then expects a mandatory trailing semicolon. Missing semicolon is a parse error. Returns `cubec_statement_expression_t` with `expression` field. Returns NULL if `read_expression` returns NULL (no expression to form a statement).
- `read_statement` (statement.c) — Statement dispatcher. Tries each statement parser in order: `read_statement_block` first (has distinguishing prefix `{`), then `read_statement_declaration` (has distinguishing prefix `var`), then `read_statement_declaration_type` (has distinguishing prefix `type`), then `read_statement_function` (has distinguishing prefix `func`/`export`/`inline`/`extern`), then `read_statement_import` (has distinguishing prefix `import`), then `read_statement_return` (has distinguishing prefix `return`), then `read_statement_empty` (has distinguishing prefix `;`), then `read_statement_expression` as the fallback (no distinguishing prefix — any expression can start it, so it must be tried last). Uses `TRY_LOCAL(onerror, ...)` to catch errors from sub-parsers; on error, returns NULL. Returns the first successful parse, or NULL if no statement matches.
- `read_statement_declaration` (statement_declaration.c) — Declaration statement (`[export|extern|builtin|comptime] var <declarator> ;`). Parses a single variable declarator: `<identifier> [: <type>] [= <expression>]`. Modifiers: `export` (exported from module), `extern` (external linkage, no initializer, requires type annotation), `builtin` (compiler-provided, no initializer, requires type annotation), `comptime` (compile-time evaluated, requires initializer, mutually exclusive with `extern` and `builtin`). `export` and `builtin` are orthogonal (can combine). `export` and `comptime` are orthogonal (can combine). `extern` is mutually exclusive with `export`, `builtin`, and `comptime`. `builtin` and `comptime` are mutually exclusive. Extern/builtin declarations must not have `= expression`. Comptime declarations must have `= expression`. Returns `cubec_statement_declaration_t` with `is_export`, `is_extern`, `is_builtin`, `is_comptime` (bools) and `declarator` (single `declaration_variable_t` node) fields.
- `read_statement_declaration_type` (statement_declaration_type.c) — Type alias declaration (`[export|builtin] type Name[<generic_params>] [= <type_expression>];`). Modifiers: `export` (exported from module, orthogonal with builtin), `builtin` (compiler-provided, no `= type_expression` body). For builtin types, `type_value` is NULL. Returns `cubec_statement_declaration_type_t` with `is_export`, `is_builtin` (bools), `name` (identifier), `params` (vec of `cubec_generic_param_t`, may be NULL), and `type_value` (type expression, NULL for builtin) fields.
- `read_statement_import` (statement_import.c) — Import statement (`import <module_name> [as <alias>] from "<path>";`). Parses module import with optional `as` alias. Returns `cubec_statement_import_t` with `module_name` (identifier node), `alias` (optional identifier node, NULL if no `as`), and `path` (string literal node) fields. Returns NULL if current token is not `import`. THROW errors on missing module name, missing `from` keyword, missing path, or missing semicolon.
- `read_function_argument` (function_argument.c) — Parses a single function parameter: `[...]<identifier> [: <type>]`. Supports `...` prefix for pack expansion parameters (`...args: Args`). The identifier is parsed via `read_literal_identifier`. The optional type annotation is parsed via `read_expression_type`. Returns `cubec_function_argument_t` with `identifier`, `type` (nullable), and `is_rest` (bool) fields. Returns NULL if current token is not `...` or an identifier.
- `read_statement_function` (statement_function.c) — Parses function declaration statement. Delegates to `read_expression_function` for the actual `func` parsing, then validates the result: statement functions must have a name, cannot have captures, and C-style variadic `...` is only allowed in extern functions. Handles modifier parsing (`export`/`inline`/`extern`/`builtin`/`comptime`) and mutual exclusion checks (`export`+`extern`, `inline`+`extern`, `builtin`+`extern`, `comptime`+`extern`, `comptime`+`builtin`) before delegation. Comptime functions must have a body. After `read_expression_function` returns, extracts fields from the expression function node (transferring ownership) and creates a `cubec_statement_function_t` node. Returns `cubec_statement_function_t` with `is_export`, `is_inline`, `is_extern`, `is_builtin`, `is_comptime`, `is_c_variadic` (bools), `name`, `generic_params` (nullable vec), `arguments` (vec with auto_dispose=true), `return_type` (nullable), `body` (nullable) fields. Returns NULL if current token is not a function declaration prefix.
- `read_expression_function` (expression_function.c) — Universal func parser that handles both anonymous and named function expressions: `func [|<captures>| | <name>] [<generic_params>] (<params>) [-> <return_type>] { <body> } | ;`. After `func` keyword, detects `|`/`||` (capture list) or identifier (function name) or falls through to `[`/`(` (anonymous, no captures, no name). Name is nullable (present for named functions, NULL for anonymous). Capture list is optional: `||` (empty, tokenized as single `||` by lexer, captures remains NULL), `|x, y|` (non-empty), or omitted entirely. Each capture is identifier-only. Generic params, function params, and return type follow standard rules. Parameter list supports C-style variadic `...` (stored in `is_c_variadic`, validity checked by caller). Body: named functions allow `;` (body=NULL), anonymous functions require `{ body }`. Returns `cubec_expression_function_t` with `name` (nullable), `captures` (nullable vec of `cubec_function_capture_t`), `generic_params` (nullable vec), `arguments` (vec with auto_dispose=true), `return_type` (nullable), `body` (nullable), `is_c_variadic` (bool) fields. Returns NULL if current token is not `func` keyword. Supports postfix: immediate call `func |x| (a: i32): i32 { return x + a; }(42)`, member access `func || ():Vec[i32] { }.field`, assignment `var f = func |x| () { };`.
- `read_function_capture` (function_capture.c) — Parses a single capture item: `<identifier>`. Captures are identifier-only. Returns `cubec_function_capture_t` with `identifier` field. Returns NULL if current token is not an identifier.
- `read_statement_return` (statement_return.c) — Parses return statement: `return [<expression>] ;`. Expression is optional (bare `return;` has `expression = NULL`). Expression parsed via `read_expression`. Returns `cubec_statement_return_t` with `expression` (nullable) field. Returns NULL if current token is not `return` keyword.
- `read_declaration_variable` (declaration_variable.c) — Variable declarator (`<identifier> [: <type>] = <expression>`). Parses a single variable declarator with optional type annotation and required initializer expression. The type is parsed via `read_expression_type`. Returns `cubec_declaration_variable_t` with `identifier`, `type` (nullable), and `expression` fields.
- `read_program_node` — Top-level entry, parses statements using a whitelist approach: `statement_import` (`import`), `statement_declaration` (`[export|extern|builtin|comptime] var`), `statement_declaration_type` (`[export|builtin] type`), `statement_function` (`[export] [inline] func` | `[extern] func` | `[builtin] func` | `[comptime] func`), `statement_empty` (`;`). `statement_expression` is NOT supported at program level (only within blocks). Uses `TRY_LOCAL(onerror, ...)` to catch errors from sub-parsers.
- `read_expression_type` (expression.c) — **Now identical to `read_expression`**. Type and value expressions share a unified parsing path. Composite types (pointer/slice/array/qualifier/function type) greedily consume their inner type expression, including ternary: `*a ? b : c` → `pointer(ternary(a, b, c))`, `func(i32) -> A ? B : C` → `func(i32) -> ternary(A, B, C)`. Use grouping to prevent greedy consumption: `(*a) ? b : c` → `ternary(pointer(a), b, c)`. `typeof` can now be used as pointer/slice/array base type: `*typeof(x)`, `[]typeof(x)`. Namespace access (`::`) binds tighter than type constructors: `*std::vec::Vec` → `*(std::vec::Vec)`. Type constraints (`extends`/`==`/`!=`) are binary operators parsed by `read_expression_binary`.
- `read_expression_type_function` (expression_type_function.c) — Parses function type expressions `func(<type_list>) -> <return_type>`. Parameters are type-only (no names), unlike function definitions which use `name: type`. Return type uses `->` (not `:`) to distinguish from function definitions. Parameters and return type are parsed via `read_expression_type` (greedy — consumes ternary/constraint). Greedy behavior: `func(i32) -> A ? B : C` → `func(i32) -> ternary(A, B, C)` (return type greedily consumes ternary). Use grouping for the alternative: `(func(i32) -> A) ? B : C`. Supports: no params `func() -> i32`, multiple params `func(i32, i32) -> void`, C-style variadic `func(i32, ...) -> void`, pack expansion `func(...Args) -> R`, complex types `func(*i32) -> *i32`, nested function types `func(func(i32) -> i32) -> void`. Can be wrapped by pointer/slice/const/volatile: `*func(i32) -> i32` (pointer to function), `const func(i32) -> i32`. Returns `cubec_expression_type_function_t` with `parameters` (vec of node_t type expressions, auto_dispose), `return_type` (nullable node_t), `is_c_variadic` (bool) fields. Returns NULL if current token is not `func` keyword or `(` does not follow `func`, or if named parameter pattern detected (`identifier:` → function expression).
- `read_expression_type_qualifier` (expression_type_qualifier.c) — Parses const/volatile type qualifier expressions. Merged from the former separate `const` and `volatile` parsers into a single node with `is_volatile` flag. A standalone prefix type modifier at the same level as pointer/slice/array. The underlying type is parsed via `read_expression_type` (greedy — consumes ternary). Placed before pointer in `read_atom` so that `const * i32` parses as `const(pointer(*i32))` rather than `*` consuming `const` as a qualifier. Greedy consumption: `const a ? b : c` → `const(ternary(a, b, c))`. Supports nesting: `const const i32`. Can combine: `const volatile i32` → `const(volatile(i32))`, `volatile const i32` → `volatile(const(i32))`. Use grouping to prevent greedy: `(const a) ? b : c` → `ternary(const(a), b, c)`. Returns `cubec_expression_type_qualifier_t` with `type` and `is_volatile` (bool) fields. Returns NULL if the current token is not the keyword `const` or `volatile`.
- `read_expression_typeof` (expression_typeof.c) — Parses compile-time type computation expression `typeof(<expression>)`. Available in both type expression context (via `read_type_expression_primary`) and value expression context (via `read_atom`). In type expressions, `typeof(x)` can be used as pointer/slice/array base type: `*typeof(x)`, `[]typeof(x)`. In value expressions, `typeof(x)` is an atom that supports full postfix chaining: `.field` (member access), `::method` (namespace access), `[i32]` (generic instantiation), `()` (call). The inner expression is parsed via `read_expression`. Returns `cubec_expression_typeof_t` with `expression` field. Returns NULL if the current token is not the `typeof` keyword. THROW errors on missing `(`, missing `)`, or missing inner expression.

### Not Yet Implemented
All statement and declaration types have parser implementations, including `comptime` blocks/if/for.

## Semantic Analysis Engine (src/engine/)

### Architecture

9-file split: `checker.c` (lifecycle + pass orchestration), `checker_collect.c` (Pass 1: symbol collection), `checker_check_stmt.c` (Pass 2: type checking), `checker_evaluate.c` (Pass 2: comptime evaluation + type resolution), `builtin.c` (builtin registry: dynamic table, validation, dispatch), `comptime_eval.c` (evaluator lifecycle), `comptime_eval_expr.c` (expression evaluation), `comptime_eval_stmt.c` (statement execution), `comptime_alloc.c` (virtual memory). All functions ≤ 50 lines. Design doc: `docs/semantic-design.md`.

### Type System (semantic_type_t)

Two-layer representation: `semantic_type_t` wraps AST type nodes with semantic info. Structural equivalence for type comparison. Pointer decay rules. Built-in types: `builtin_i8`~`builtin_u64`, `builtin_f16`~`builtin_f64`, `builtin_bool`, `builtin_void`, etc. Type layout computation via `type_layout_compute`.

**const/volatile qualifier**: `TYPE_QUALIFIER` has `is_const` and `is_volatile` flags. `*const T` → `POINTER(QUALIFIER(const, T))` (pointer to const T, C: `const T*`). `const *T` → `QUALIFIER(const, POINTER(T))` (const pointer, C: `T* const`). Utility functions: `semantic_type_is_const()`, `semantic_type_is_volatile()`, `semantic_type_strip_qualifier()`. Const enforcement: assignment to const lvalue errors, member/deref const propagation, `is_mutable` set based on `!semantic_type_is_const()`. Implicit conversion allows `T → const T` but not `const T → T`. Comptime eval enforces const but ignores volatile.

**Parameter pack type**: `TYPE_GENERIC_PACK` represents a variadic pack type, containing `element_types` (vec of semantic_type_t). Used when a generic parameter is declared with `...` prefix. Pack types expand to zero or more concrete types during instantiation. The comptime value layer uses `COMPTIME_VALUE_PACK` with an `elements` vec to represent pack values at compile time.

### Symbol Table (scope_t)

Chain-of-responsibility scope model. `scope_lookup` returns `SYMBOL_NAME_KNOWN` for imported values (cannot be used at compile time). TDZ (temporal dead zone) multi-pass checking. Symbols have `is_builtin` flag set when builtin declaration passes validation against the builtin table. `undefined` literal: `CUBEC_NODE_LITERAL_UNDEFINED` is a valid initializer for `var` declarations with explicit type annotation; the variable enters `SYMBOL_TDZ` state. Non-extern/non-builtin `var` declarations require an initializer (`var x: i32;` is an error). Using `undefined` as a standalone expression is an error.

### Builtin Registry (builtin_table_t)

Dynamic registry (`builtin.h`/`builtin.c`) mapping names to `builtin_entry` (kind + type + dispatch ID). Initialized in `checker_init` via `builtin_table_init_defaults` which registers `assert` as `BUILTIN_FUNC` with `BUILTIN_DISPATCH_ASSERT`. Builtin declarations go through normal checker flow (type resolution, generic param handling) then are validated against the table: unknown builtin → error, kind mismatch → error, signature mismatch → error, match → `sym->is_builtin = true`. Comptime eval uses `callee_sym->is_builtin` + dispatch ID instead of hardcoded name checks.

### Checker Passes

1. **Pass 1 (collect)**: `checker_collect` — symbol collection, scope building
2. **Pass 2 (check + evaluate)**: `checker_check_stmt` (type checking) + `checker_evaluate` (comptime evaluation, type resolution)

### Diagnostics

Rustc-style error messages: source line + `^` caret span annotations via `diagnostic_list_push`.

## Comptime Evaluator (AST Interpreter)

### Value Representation (comptime_value_t)

12 value kinds: `NIL`, `BOOL`, `INT` (i8..i64, u8..u64 with width+signedness), `FLOAT` (f16/f32/f64 with width), `CHAR`, `STRING`, `TYPE` (typeof result), `POINTER` (virtual address), `COMPOSITE` (struct/union instance, contiguous raw-byte layout `uint8_t *data` + `data_size` + `element_type`), `FUNCTION` (closure: env + AST body + param names), `PACK` (variadic pack: `elements` vec), `ERROR`.

API: `comptime_value_create_*` constructors, `comptime_value_is_truthy`, `comptime_value_equals`, `comptime_value_clone` (deep copy via `value_clone`), `comptime_value_as_i64/as_u64/as_f64`, `comptime_value_read_field/write_field` (raw byte field access by offset), `comptime_value_get_field/set_field` (named field access for struct), `comptime_value_get_index/set_index` (array element access).

### Virtual Memory (comptime_allocator_t)

uint64_t address space (0 = null). `strmap_t` for allocations + lifetimes. Scope-based lifetime management: `enter_scope`/`leave_scope` frees allocations at depth ≥ current. Dangling pointer detection: `comptime_alloc_read` returns NULL for freed/invalid addresses. `comptime_alloc_write` returns false for invalid addresses.

### Variable Environment (comptime_env_t)

Chain-of-responsibility scope model with `strmap_t` bindings (value_auto_dispose=false). `bind(name, value)`, `lookup(name)`, `update(name, value)`. Global env pre-binds `true`, `false`, `nil`. Temporaries tracked per-scope via `temporaries` vec (auto_dispose=true), released on scope exit.

### Control Flow Signals

`COMPTIME_SIGNAL_NONE/RETURN/BREAK/CONTINUE/ERROR`. Signal propagates through statement execution, handled by caller (e.g., function call extracts return value, loop handles break/continue).

### Expression Evaluation (_eval_expr)

Covers: literal numeric/string/char/identifier, binary ops (10 precedence levels), prefix unary (!/-/~), assignment (incl. composite field writeback), function call, member access, namespace access, ternary, group, sizeof/alignof/typeof, function closure, initialize list, comma, slice (string + array), deref (`*`), addr (`&`), type nodes.

### Statement Execution (_exec_stmt)

Covers: block (with scope), expression, return, if, while, do-while, for, foreach, declaration, function, break, continue, empty, defer, switch, comptime block/if/for. Type declaration nodes are skipped (handled by checker_evaluate).

### Safety Limits

- `COMPTIME_MAX_LOOP_ITERATIONS = 1024`
- `COMPTIME_MAX_CALL_STACK_DEPTH = 256`

### Checker Integration

`checker_t` owns `comptime_eval_t`. `_evaluate_comptime_block/if/for` delegate to evaluator. `_evaluate_variable` binds comptime var values to env. `_evaluate_function` binds comptime functions with body to env (param names as C strings, not string_t objects).

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
1. **cubecc** — Compiler executable (links `src/core/`, `src/engine/`, `src/cubec/`, `src/reader/`, `src/writer/`, `src/c/` + `src/main.c`). Note: `reader/`, `writer/`, `c/` directories don't exist yet — future modules for source reading, code generation, and C backend.
2. **cubec_test** — Test executable (links all source files + test/*.cpp, depends on GTest)

### Platform-Specific Linking
- Windows + Clang: extra link `clang_rt.builtins-x86_64`
- Linux + GCC: `-pthread` via `Threads::Threads`

### ICU Data Handling
- ICU common data stored as binary `third_party/icudt74l.dat` (~30MB, under Git 100MB limit)
- At **build time**, CMake converts `.dat` → C byte array via `cmake/bin_to_c.ps1` (Windows) or `xxd` (Unix)
- Generated file written to `build/icu_data_gen.c` (not tracked by Git, auto-regenerated only when `.dat` changes)
- This replaces the previous ~181MB `src/icu_data_gen.c` which exceeded Git's file size limit

## Testing

- Framework: Google Test + C++20
- Helper: `test_allocator` RAII class in `test/common/test_common.h`
- Total: 1304 test cases

### Core Tests
- `dt_allocator.cpp` (12 cases) — create/destroy, alloc/free, zero-size, NULL-free, multi-alloc, type create, value introspection, clone, move
- `dt_vec.cpp` (20 cases) — create/resize, push/get, pop, set, insert, remove, resize, get_data, initial capacity + 10 iterator cases (iteration, empty, get, set, remove, remove_first, remove_last, remove_exhausted, traverse_remove_all, single_element)
- `dt_list.cpp` (20 cases) — full list operations using iterator-based get/set/remove; no indexed access APIs
- `dt_rbtree.cpp` (14 cases) — insert/find, duplicates, remove, clear, iteration, auto_dispose, clone, move
- `dt_map.cpp` (16 cases) — hash threshold (15 entries), RB-tree conversion threshold (20 entries), auto_dispose, clone, move
- `dt_string.cpp` (15 cases) — all string operations including Unicode, nconcat, binary data
- `dt_location.cpp` (12 cases) — `location_is` (exact match, short-vs-long keywords, prefix partial match, empty strings, case sensitivity) + `location_get` (extract text, empty span)
- `dt_token.cpp` (56 cases) — all token types, all 40 keywords, all numeric formats, all escape sequences, comments, symbols

### Cubec Tests
- `dt_literal_char.cpp` (5 cases)
- `dt_literal_identifier.cpp` (5 cases)
- `dt_literal_numeric.cpp` (11 cases) — integer/float parsing, base prefixes (hex/oct/bin), type suffixes (i8~u64, f32/f64), scientific notation
- `dt_literal_string.cpp` (12 cases)
- `dt_expression_binary.cpp` (40 cases) — 4 prefix unary operators (!/+/-/~), chained prefix (!!x, --n), prefix+member (!obj.field), 18 binary operators across 10 precedence levels, 4 precedence interaction tests, 3 postfix unary interaction tests (ptr.*, x.&, binary with postfix), 2 whitespace handling, 1 member+binary, 1 full chain
- `dt_expression_group.cpp` (13 cases) — basic parenthesized expression, group with binary inner, empty group error, unclosed group error, nested groups, group with identifier, group as LHS of binary, non-group returns NULL
- `dt_expression_member.cpp` (8 cases) — single member access, chained access, consume all tokens, member on string literal, error on missing dot, error on non-identifier field, not a member (no dot), empty source
- `dt_expression_namespace_access.cpp` (11 cases) — single namespace access (`std::vec`), chained (`std::vec::Vec`), spaces, namespace+generic (`std::vec::Vec[i32]`), static member access (`std::Vec::create`), in normal expression (`a + std::Vec::create() + b`), mixed `::`+`.` (`std::Vec::new().field`), in type expression (`*std::vec::Vec`), consume all tokens, clone, move
- `dt_expression_spread.cpp` (12 cases) — spread identifier, spread numeric, spread with spaces, spread member access, spread group, spread binary value, non-spread returns NULL, single dot returns NULL, double dot returns NULL, spread without value, dots not at start, spread with postfix unary
- `dt_expression_call.cpp` (15 cases) — zero args, single arg, two args, numeric arg, binary-expr arg, single spread arg, mixed spread+regular, multiple spreads, chained call `foo()()`, call→member chain, member→call chain, group-as-callee, non-call not triggered, unclosed paren error, trailing comma error
- `dt_expression_generic_instantiation.cpp` (15 cases) — basic instantiation `foo[a]`, multi-arg `foo[a,b]`, instantiation on literal, instantiation on call, instantiation→call chain `fn[a]()`, instantiation→member chain `fn[a].field`, call→instantiation chain `foo()[a]`, instantiation→instantiation chain `fn[a][b]`, spread in generic args, group as callee, nested generic groups, non-generic not triggered (no `[`), unclosed bracket error, trailing comma error, generic on `this`
- `dt_expression_ternary.cpp` (8 cases) — simple ternary `a ? b : c`, ternary with binary condition `x + a ? b : c`, missing `?` fallback, missing `:` error, missing consequent error, missing alternate error, nested ternary `a ? b ? c : d : e`, complex alternate `a ? b : c + d`
- `dt_expression_comma.cpp` (12 cases) — basic comma `a, b`, numeric literals, right-associative chaining (`a, b, c` → `comma(a, comma(b, c))`), assignment in left/right position, comma with binary/call expressions, non-comma fallback (returns operand directly)
- `dt_expression_type.cpp` (63 cases) — simple identifier types (`i32`, `Vec`), generic instantiation with single/multiple arguments (`Vec[i32]`, `Map[string, i32]`), type parameters (`Option[T]`), single/chained namespace access (`std::vec`, `std::vec::Vec`), namespace with generic (`std::vec::Vec[i32]`), generic arguments as namespace access types (`Vec[std::vec::Vec]`, `Map[std::vec::Vec, std::str::String]`), mixed arguments (`Pair[i32, std::vec::Vec]`), deeply nested generics with namespace, non-identifier returns NULL (string/numeric literals), empty brackets handling, underscore prefix identifiers, whitespace handling, token consumption verification + pointer declaration tests (`* i32`, `* const i32`, `* volatile i32`, `* const volatile i32`, `** i32`, `* Vec[i32]`, `* std::vec::Vec`, `Vec[* i32]`, `* * i32`, `* std::vec::Vec[i32]`) + slice declaration tests (`[] i32`, `[] const i32`, `[] volatile i32`, `[] const volatile i32`, `[] Vec[i32]`, `[] std::vec::Vec`, `Vec[[] i32]`) + array declaration tests + reordered/repeated qualifier tests + nested type group test
- `dt_expression_type_ternary.cpp` (31 cases) — type-level ternary expressions (parsed via `read_expression_type` = `read_expression`): simple `a ? b : c`, type_group condition `(a) ? b : c`, expr_group condition `(1) ? a : b`, nested with group `(a ? b : c) ? d : e`, deeply nested, pointer greedy ternary `* a ? b : c` → `pointer(ternary(a,b,c))`, pointer condition in group, slice greedy ternary `[] a ? b : c` → `slice(ternary(a,b,c))`, array greedy ternary `[10] a ? b : c` → `array(ternary(a,b,c))`, pointer to ternary via group `* (a ? b : c)`, slice to ternary via group `[] (a ? b : c)`, array to ternary via group `[10] (a ? b : c)`, missing `?` fallback, missing `:` error, missing consequent error, missing alternate error, generic consequent `a ? Vec[i32] : f32` + group interaction tests + 7 type-constraint-as-condition tests: `extends` constraint (`T extends U ? X : Y` — binary `extends` as condition), `==` constraint (`T == U ? X : Y`), `!=` constraint (`T != U ? X : Y`), pointer right in group (`T extends (* U) ? X : Y`), enable_if pattern with generic consequent (`T extends K ? Vec[T] : f32`), bare constraint without `?` is error (`T extends U` → THROW), nested constraint ternary in group (`(a extends b ? c : d) ? e : f`)
- `dt_expression_type_const.cpp` (13 cases) — const type qualifier expressions: simple `const i32`, nested `const const i32`, non-const returns NULL, const with pointer `const * i32` → type_qualifier(pointer), const with slice `const [] i32`, const with array `const [10] i32`, const with const pointer `const * const i32`, const with generic `const Vec[i32]`, const with namespace `const std::vec::Vec`, const with type_group wrapping ternary `const (a ? b : c)`, const greedy ternary `const a ? b : c` → `const(ternary(a,b,c))`, clone and move
- `dt_expression_type_volatile.cpp` (15 cases) — volatile type qualifier expressions: simple `volatile i32`, nested `volatile volatile i32`, non-volatile returns NULL, volatile with pointer `volatile * i32`, volatile with slice `volatile [] i32`, volatile with array `volatile [10] i32`, volatile with volatile pointer `volatile * volatile i32`, volatile with generic `volatile Vec[i32]`, volatile with namespace `volatile std::vec::Vec`, volatile with type_group wrapping ternary `volatile (a ? b : c)`, volatile greedy ternary `volatile a ? b : c` → `volatile(ternary(a,b,c))`, const+volatile combinations `const volatile i32` and `volatile const i32`, clone and move
- `dt_expression_type_constraint.cpp` (11 cases) — type constraint expressions (parsed as binary ops): simple `extends` (`T extends U` → EXPRESSION_BINARY with opt "extends"), `==` (`T == U` → EXPRESSION_BINARY with opt "=="), `!=` (`T != U` → EXPRESSION_BINARY with opt "!="), fallback identifier, generic right operand (`T extends Vec[i32]`), namespace access right (`T == std::vec::Vec`), pointer right (`T != * const i32`), `extends` as ternary condition (`T extends U ? X : Y`), `==` as ternary condition (`T == i32 ? Vec[T] : T`), `!=` as ternary condition (`T != f64 ? f32 : T`), typeof+extends in value ternary (`(typeof(a) extends i32) ? 1 : 2`), pointer to constraint-ternary via group (`* (T extends U ? X : Y)`)
- `dt_expression_type_function.cpp` (25 cases) — function type expressions: simple `func(i32) -> i32`, two params, no params, pointer param/return, generic param, namespace param, C-style variadic, variadic only, function type as param, pointer to function, slice of function, const/volatile function, consume all tokens, with spaces, non-func returns NULL, error cases (missing open paren, close paren, arrow, return type, trailing comma), greedy ternary return `func(i32) -> A ? B : C`, greedy ternary param `func(A ? B : C) -> i32`, clone, move
- `dt_func_type_extends.cpp` (3 cases) — typeof + function type + extends interaction: `typeof(fn) == func(i32) -> i32` (binary == with function type), greedy ternary `typeof(fn) == func(i32) -> i32 ? Vec[i32] : f32` (func return type greedily consumes ternary → binary at top), grouped ternary `typeof(fn) == (func(i32) -> i32) ? Vec[i32] : f32` (group disambiguation → ternary at top)
- `dt_expression_initialize_field.cpp` (12 cases) — initialize field expressions: basic `.name = 42`, string/identifier/binary expression values, spaces, no-dot returns NULL, no-equals returns NULL, missing identifier after dot, numeric after dot, clone, move, nested expression in value, consume all tokens
- `dt_expression_initialize_list.cpp` (23 cases) — initialize list expressions: anonymous empty `.{}`, typed empty `.Vec{}`, typed field items `.Vec{.x=1, .y=2}`, typed positional items `.Vec{1, 2, 3}`, anonymous field/positional items, nested initialize_list as expression `.{.Test{}}`, field vs expression disambiguation `.{.Test=123}`, postfix member chain `.Vec{1,2}.field`, in binary expression `1 + .Vec{1,2}`, trailing comma allowed `.Vec{1, }`, unclosed brace error, mixed items error, clone, move, single positional/field item, no-dot returns NULL, dot+identifier without brace returns NULL, nested typed with fields `.{.Inner{.a=1}}`, typed namespace access type `.std::vec::Vec{1,2}`, typed generic instantiation `.Vec[i32]{1,2}`, typed pointer type `.*i32{1,2}`
- `dt_expression_typeof.cpp` (14 cases) — typeof expressions: typeof with identifier `typeof(x)`, typeof with binary expression `typeof(a+b)`, typeof with function call `typeof(foo())`, typeof as type expression, typeof with namespace access `typeof(File)::open`, typeof as pointer base `*typeof(x)`, typeof as slice base `[]typeof(x)`, typeof with generic instantiation `typeof(Vec)[i32]`, consume all tokens, missing `(` error, missing `)` error, not typeof returns NULL, clone, move
- `dt_expression_sizeof.cpp` (13 cases) — sizeof expressions: sizeof with identifier `sizeof(x)`, sizeof with binary expression `sizeof(a+b)`, sizeof with function call `sizeof(foo())`, via read_expression, sizeof with member access `sizeof(x).field`, sizeof in binary `sizeof(x) + sizeof(y)`, consume all tokens, missing `(` error, missing `)` error, not sizeof returns NULL, clone, move
- `dt_expression_alignof.cpp` (13 cases) — alignof expressions: alignof with identifier `alignof(x)`, alignof with binary expression `alignof(a+b)`, alignof with function call `alignof(foo())`, via read_expression, alignof with member access `alignof(x).field`, alignof in binary `alignof(x) + alignof(y)`, consume all tokens, missing `(` error, missing `)` error, not alignof returns NULL, clone, move
- `dt_expression_function.cpp` (21 cases) — function expressions: no captures no params `func || () { }`, simple captures `func |x, y| () { }`, empty captures with params, generic with/no captures, with params, no return type, complex return type, no params, immediate call `func |x| (...)(42)`, chained member, assign to var, clone, clone with captures, move, missing pipe error, missing close pipe error, missing body error, not func returns NULL, via read_expression, consume all tokens
- `dt_statement_expression.cpp` (10 cases) — expression statements: simple identifier `foo;`, numeric literal `42;`, binary expression `a + b;`, function call `foo();`, namespace access call `std::Vec::create();`, consume all tokens, missing semicolon error, semicolon only returns NULL, clone, move
- `dt_statement_block.cpp` (11 cases) — empty block `{}`, single empty statement `{;}`, single expression statement `{ foo(); }`, multiple statements `{ foo(); bar; ; }`, nested blocks `{ { ; } }`, via read_statement dispatcher, no brace returns NULL, unclosed brace is error, unexpected token in block is error, clone, move
- `dt_statement_declaration.cpp` (31 cases) — single declarator without/with type, complex expression, pointer type, initialize list (anonymous/typed/field items/nested), consume all tokens, clone, move, export single/type, non-export, export clone/move, extern var, builtin var, export+builtin var, builtin+export var (order-independent), extern+export mutual exclusion error, extern+builtin mutual exclusion error, extern var with initializer error, builtin var with initializer error, extern var without type error, extern var clone, builtin var move, comptime var, comptime var without initializer error, export+comptime var, comptime+export var (order-independent), builtin+comptime mutual exclusion error, comptime+builtin mutual exclusion error, extern+comptime mutual exclusion error, comptime var clone, comptime var move
- `dt_statement_declaration_type.cpp` (20 cases) — type alias declarations: simple alias (`type MyInt = i32`), generic alias (`type Vec3[T] = ...`), multi-param (`type Pair[A, B] = ...`), complex nested type, consume all tokens, clone, clone with generic params, move, rest param single/after regular/with constraint/clone, regular param is not rest, export simple/generic/pointer type, non-export, export clone/move, builtin type no body, export builtin type, builtin type no params, builtin type clone/move
- `dt_statement_import.cpp` (15 cases) — import statements: simple import (`import std from "std"`), import with alias (`import vec as v from "std/vec"`), relative path (`import io from "./io"`), parent path (`import parent from "../parent"`), multi-segment path (`import vec from "std/vec"`), consume all tokens, missing `from` keyword error, missing semicolon error, non-import returns NULL, missing module name error, missing path error, clone, move, via `read_statement` dispatcher, via `read_program_node`
- `dt_statement_function.cpp` (44 cases) — function declarations: basic function, no params, no return type (void), single/multiple params, generic single/multiple/rest params, export/inline/extern/builtin/comptime modifiers, export+inline combined, extern C-style variadic (`...`), pointer/slice/generic/no-type params, empty body, body with statements, no body semicolon (interface style), missing name/open paren/close paren errors, export+extern/extern+inline conflict errors, C variadic in non-extern error, builtin func, export+builtin func, builtin+extern mutual exclusion error, comptime func, comptime func without body error, export+comptime func, inline+comptime func, builtin+comptime mutual exclusion error, extern+comptime mutual exclusion error, clone, clone generic, move, clone extern, via read_statement, via read_program_node, consume all tokens
- `dt_generic_pack.cpp` (15 cases) — variadic generics (parameter packs): pack param declaration, pack must be last, only one rest param, single explicit type arg, empty pack expansion `foo[]()`, mixed regular and pack params, pack in function type `func(...Args) -> R`, pack inference from call, pack expansion in params `...args: Args`, decorator pattern e2e `wrap[R, ...Args]`, pack with constraint `...Args extends i32`, pack spread in init list, empty pack init, mixed init, pack overflow init
- `dt_generic_inference.cpp` (19 cases) — generic type inference from call arguments: single i32/f64 inference, two-param inference, same-param consistency, pointer/slice param inference, mismatch error, unresolved param error, constraint interface pass/fail, constraint structural pass/fail, constraint generic instance, constraint pointer, constraint wildcard skips, infer with constraint pass/fail

### Engine Tests
- `dt_undefined.cpp` (9 cases) — undefined literal: typed undefined init, no-type error, standalone expr error, var-no-init error, extern-no-init ok, builtin-no-init ok, TDZ use before assign, assign-then-use, pointer type undefined
- `dt_builtin.cpp` (9 cases) — builtin registry: table create/dispose, assert lookup, unknown lookup, correct declaration, unknown builtin error, signature mismatch error, kind mismatch error, e2e assert execution, non-builtin not marked
- `dt_comptime_value.cpp` (23 cases) — value creation/disposal for all 12 kinds, truthiness, equals, clone deep copy, numeric conversions (as_i64/as_u64/as_f64)
- `dt_comptime_alloc.cpp` (10 cases) — virtual memory lifecycle, allocate+read, write overwrite, read/write null/unknown addr, free makes addr invalid, scope enter/leave frees allocations, nested scopes, free null addr noop
- `dt_comptime_eval.cpp` (38 cases) — evaluator: literal numeric/string/char/bool/nil, arithmetic ops, comparison ops, logical ops, bitwise ops, ternary, variable declaration+access, if/else, for loop, function call, break/continue, typeof/sizeof/alignof, group expression, comma expression, string/composite slice, do-while, foreach, composite field assignment
- `dt_statement_interface.cpp` (18 cases) — interface declarations: basic interface, interface with method, interface with type member, generic single/multi params, method no return type, method generic, method pointer/slice return type, export/non-export interface, export with method, clone, move, consume all tokens, via read_statement, via read_program_node, type member with generic
- `dt_expression_type_interface.cpp` (14 cases) — anonymous interface type expressions: simple empty, with method, with type and method, generic single/multi, pointer/slice/const wrapped, consume all tokens, non-interface returns NULL, clone, move, via read_atom, via read_expression
- `dt_statement_struct.cpp` (16 cases) — struct declarations: basic struct, empty, instance fields, pub field, generic single/multi, static var field, type member, method, export/non-export, clone, move, consume all tokens, via read_statement, via read_program
- `dt_expression_type_struct.cpp` (14 cases) — anonymous struct type expressions: simple empty, instance field, pub field, static var + instance field, generic, pointer/slice/const wrapped, consume all tokens, non-struct returns NULL, clone, move, via read_atom, via read_expression

## Module System (模块系统)

### Import Syntax

Use `import` keyword for module imports, with namespace-style access:

```c
import <module_name> from "<module_path>";
import <module_name> as <alias> from "<module_path>";
```

- `<module_name>`: The name used to access the module in current file
- `<module_path>`: Module path (see path resolution rules below)
- `as <alias>`: Optional renaming

**Examples**:

```c
import std from "std";                 // Import std module
import io from "./io";                 // Import io.cubec in current directory
import vec as v from "std/vec";        // Rename import
std::println("hello");                  // Access via namespace (::)
```

### Export Syntax

Use `export` keyword prefixed to declarations:

```c
export func add(a: i32, b: i32): i32 { ... }
export struct Point { x: f64, y: f64 }
export type Array[T, N] = [N]T;
export const PI: f64 = 3.14159;
export var global_mutable: i32 = 42;
```

### Default Private Principle

All declarations within a module are **not exported by default**. Only declarations marked with `export` are accessible to other modules.

### Path Resolution Rules (Node.js ES Module Style)

| Path Format | Rule |
|-------------|------|
| `./xxx` | Relative path (relative to current file's directory) |
| `../xxx` | Relative path (parent directory) |
| `xxx` | Logical path (resolved from project root or module base path) |

**Examples**:

```c
import std from "std";          // Logical path → std.cubec
import io from "./io";          // Relative path → io.cubec
import parent from "../parent"; // Relative path → ../parent.cubec
import vec from "std/vec";      // Logical path → std/vec.cubec
```

### Module Entry Point

`import xxx from "path";` looks for `path.cubec` as the module entry.

| import statement | Lookup file |
|------------------|-------------|
| `import x from "foo";` | `foo.cubec` |
| `import x from "./bar";` | `./bar.cubec` |
| `import x from "foo/bar";` | `foo/bar.cubec` |

### Import Renaming

Use `as` keyword to rename imported modules:

```c
import std as s from "std";
import very_long_module_name as m from "some/module";

s::println("hello");  // Use renamed name
```

### Cyclic Dependencies

- **Allowed**: Modules can import each other
- **Compile-time constants only**: Global variables must be compile-time constants (no function calls)
- **Safe by design**: Circular references only occur in type declarations and compile-time constants, impossible to cause runtime uninitialized issues

```c
// a.cubec
import b from "b";
export const PI = 3.14;                          // ✅ OK
export type MyType = b.OtherType;                // ✅ OK: type reference
export var illegal = b.create();                 // ❌ ERROR: non-compile-time constant

// b.cubec
import a from "a";
export type OtherType = a.YetAnotherType;        // ✅ OK: circular type reference
export struct SomeType { ref: *a.SomeType }     // ✅ OK: struct definition
```

---

## Planned Language Features (syntax design confirmed, implementation pending)

实现顺序：enum → union → cunion → if → while → for → foreach → switch → break/continue → defer → test → decorator → comptime

### enum 枚举声明
- TypeScript 风格，编译期常量，不支持泛型
- `[export] enum <name> { <item> [: <type>] [= <value>], ... }` — 类型和值均可省略
- 匿名 enum 类型表达式：`enum { A: u8 = 1 }`
- 节点：CUBEC_NODE_STATEMENT_ENUM, CUBEC_NODE_DECLARATION_ENUM, CUBEC_NODE_ENUM_ITEM

### union 联合体声明
- Rust 风格 tagged union，字段用逗号分隔，支持泛型
- `[export] union <name> [<generic_params>] { <field>: <type>, ... }`
- 匿名 union 类型表达式：`union { ok: i32, err: *u8 }`
- 节点：需新增 CUBEC_NODE_STATEMENT_UNION 等

### cunion C 风格联合体
- C 兼容，字段用分号分隔，无 tag 字节
- `cunion <name> { <field>: <type>; ... }`
- 不支持泛型、export、匿名类型表达式

### if 条件语句
- `if(condition) { } else if(condition) { } else { }` — 条件必须括号

### for 循环
- C 风格三段式：`for(init; condition; increment) { }`

### foreach 迭代器循环
- `foreach(<lvalue>|var <identifier>[:<type>] of <expression>) <statement>` — 迭代器遍历
- 使用 `of` 关键字（非 `:`）分隔迭代变量与迭代器，避免与类型注解歧义
- 两种模式：
  - lvalue 模式：`foreach(i of items)` — 使用已有变量
  - var 模式：`foreach(var i of items)` — 声明新的可变循环变量
  - var 带类型：`foreach(var i: i32 of items)` — 声明并标注类型
- body 为单条 statement（与 if/while/for/do-while 一致），非必须 block
- 仅支持迭代器协议（对象含 `next()` 方法返回 `{value, done}`），不直接支持数组/切片/字符串

### while / do-while 循环
- `while(condition) { }` — 条件必须括号
- `do { } while(condition);` — 括号+分号结尾

### switch 分支语句
- `switch(value) { case(a, b) -> { }, else -> { } }` — 括号+逗号分隔多值
- `->` 连接 case 和 body（需新增词法 token）
- 支持表达式形式（有返回值，类似 Rust match）

### defer 延迟执行
- `defer expr();` 和 `defer { }` 两种形式

### break / continue
- 仅简单形式 `break;` / `continue;`，不支持标签

### test 测试块
- `test "name" { }` — 仅顶层使用，名称必须

### decorator 装饰器
- `[[expr]]` C++11 attribute 风格，内部是编译期表达式，求值后必须是符合要求的函数
- 多个叠加：`[[inline]] [[export]] func foo() { }`
- 可修饰：func、struct/enum/union、type、var

### comptime 编译时求值
- `comptime { }` — 独立 AST 节点
- `comptime if(condition) { } else { }` — 独立 AST 节点
- `comptime for(init; cond; incr) { }` — 编译期循环展开

## Generics System (泛型系统)

Cubec 的泛型机制基于**"推导 + 鸭子类型"**范式，采用编译期模板实现。所有泛型参数使用方括号 `[]` 语法，支持类型泛型、值泛型、约束校验、类型级运算和编译期分支。

### 设计原则

| 原则 | 说明 |
|------|------|
| 推导优先 | 泛型参数从函数实参类型自动推导，推导失败则编译报错（除非显式指定） |
| 鸭子类型 | 约束校验基于结构兼容性（A 是否具备 B 的操作），非继承链检查 |
| 无重载 | 函数名对应唯一实现，无重载决议，降低复杂度 |
| `[]` 语法 | 统一使用方括号，解析阶段即可区分泛型与比较运算符 |
| 显式传递 | 推导失败时支持显式类型实参：`parse[i32]("42")` |

### 各类型泛型支持总览

| 类型 | 泛型支持 | 推导 | 语义 |
|------|----------|------|------|
| struct | ✅ 支持 `struct Vec[T] { ... }` | ❌ 不支持推导（无构造函数，显式写 `Vec[i32]{}`） | 编译期模板实例化 |
| enum | ❌ 不支持泛型 | — | TypeScript 风格：编译期常量，成员可指定类型和值 |
| union | ✅ 支持 `union Option[T] { value: T, tag: u64 }` | — | Rust 风格：tagged union，字段+逗号分隔 |
| cunion | ❌ 不支持泛型 | — | C 风格：字段重叠存储，分号分隔，无 tag |
| interface | ✅ 支持 | — | Go/TypeScript 风格：仅存方法签名，结构型 / duck typing |
| func | ✅ 支持 | ✅ 支持从实参推导 | 泛型函数 |

### 泛型规则总览（16 条）

#### 1. 泛型参数定义

```c
// 简单形式：裸标识符，类型从实参推导
func[T](x: T): T

// 约束形式：通过 extends 限制
func[T extends Numeric](x: T): T

// Rest 参数：以 ... 为前缀，收集零个或多个类型实参
type Variadic[...Args] = i32
func[T extends Numeric, ...Rest](first: T, rest: Rest): T
```


#### 2. 无重载 + 鸭子类型 = 低复杂度

无函数重载（每函数名唯一实现），`extends` 基于"被约束方是否具备约束方要求的操作"判定，杜绝 SFINAE、偏特化等复杂性。

#### 3. 多位置同一泛型参数的统一（Unification）

```c
func[T](a: T, b: T)  // T 同时出现在 a 和 b
```

逐位置独立推导 → 鸭子类型等价判定统一。若 `T` 有 `extends` 约束，各位置均需各自满足且互相兼容。

#### 4. 嵌套解包推导

```c
func[T](x: Vec[T])       // 实参 Vec[i32] → 推断 T = i32
func[K, V](x: Map[K, V]) // 实参 Map[string, i32] → K = string, V = i32
```

递归解开外层容器的壳，匹配内层泛型参数槽位。

#### 5. 通配符 `?`

```c
func[T extends Array[?]](arr: T)  // 接受任意元素类型的 Array

// ✗ 错误：? 不能作为具体参数类型
func(x: Array[?]): bool
```

`?` 仅存于 `extends` 约束子句，校验阶段无条件通过，不作为可实例化类型。泛型是编译期模板，`?` 不会出现在生成代码中。

#### 6. 类型级三元运算符

```c
func[T](x: T): T extends Numeric ? i64 : string
```

编译期判定 `A extends B`，满足取 `X`，否则取 `Y`。复用 `extends` 语义。

#### 7. 类型相等/不等判断

```c
func[T](x: T): T == i32 ? f64 : string
func[T](x: T): T != void ? T : i32
```

- `A == B` = `A extends B && B extends A`（双向鸭子等价）
- `A != B` = `!(A == B)`

#### 8. 编译期值泛型

```c
func[N: u64](arr: [N]i32): i32
func[N: u64, T extends Array[?]](arr: [N]T): T
```

`[]` 内天然是 compile-time 上下文，无需 `comptime` 或 `var` 修饰。

#### 8a. 参数包 (`...identifier`)

```c
func foo[...Args](): void {}
func foo[T, ...Args](x: T, ...args: Args): void {}
func sum[...Args extends i32](...args: Args): i32 { ... }

// 函数类型中的参数包
func wrap[R, ...Args](fn: func(...Args) -> R): func(...Args) -> R { ... }

// 空包展开
foo[]()  // Args = []，调用等价于 foo()

// 装饰器模式
func wrap[R, ...Args](fn: func(...Args) -> R): func(...Args) -> R {
    return func |fn| (...args: Args): R { return fn(...args); };
}
```

参数包以 `...` 作为前缀，后跟标识符。解析器在读取 identifier 之前先检测 `...` 符号，检测到则设置 `is_rest = true`。规则：
- 参数包必须是泛型参数列表的最后一个参数
- 不允许出现多个参数包
- 参数包可带 `extends` 约束，每个展开的类型都必须满足
- 函数参数中使用 `...args: PackName` 声明包展开参数
- 函数类型中使用 `func(...PackName) -> R` 表达包展开函数类型
- 调用中使用 `fn(...args)` 展开参数包
- 空泛型实参列表 `foo[]()` 表示零展开

语义表示：
- 类型层：`semantic_type_t` 中 `TYPE_GENERIC_PACK`，含 `element_types` 向量
- 值层：`comptime_value_t` 中 `COMPTIME_VALUE_PACK`，含 `elements` 向量
- AST 层：`cubec_generic_param_t` 的 `is_rest`、`cubec_function_argument_t` 的 `is_rest`

#### 9. 泛型一律使用 `[]`

```c
// ✓ 正确
func[N: u64, T](arr: [N]T): T

// ✗ 错误：不存在 <> 语法
func<N: u64>(...)
```

与项目中已有的 `read_expression_generic_instantiation`（`callee[args]`）统一，避免 `<` `>` 与比较运算符歧义。

#### 10. `type` 别名支持泛型 + 类型变换

```c
// 简单别名
type MyInt = i32

// 泛型别名
type Vec3[T] = Vec[Vec[Vec[T]]]
type Pair[A, B] = struct { first: A, second: B }
```

右侧类型表达式可使用泛型参数、`extends`/`==`、三元运算符等。

#### 11. `comptime if` 编译期分支

```c
func[T](x: T): void {
    comptime if (T extends Numeric) {
        print("numeric: ", x)
    } else {
        print("non-numeric")
    }
}
```

不满足条件的分支不参与代码生成，类似 C++17 `if constexpr` 或 Zig `comptime if`。

#### 12. 全类型编译期计算

编译期支持所有基本类型（整数、浮点、bool、字符串）以及 struct 和 array。编译期指针通过安全的"虚拟指针"（map + id）实现，不暴露真实内存地址。允许调用 `extern` 声明的外部函数。

```c
extern func read_file(path: [*:0]u8): []u8

comptime var data = read_file("config.json")  // 编译期 IO
```

#### 13. `builtin` 编译器指令

`builtin` 是声明前缀修饰符，表示实现由编译器提供。支持三种声明类别：

**语法**：
```c
builtin type Name[T extends constraint?];    // 编译器内建类型变换，无 body
builtin var Name: Type;                       // 编译期内建常量，无初始化
builtin func Name(params): Type;             // 编译器内联函数，无函数体
```

**组合规则**：
- `export builtin` — 正交组合，内建且导出
- `extern builtin` — 互斥，语义冲突
- 无 body — builtin 声明全部无实现体

**内置类型变换指令**：

| builtin | 约束 | 结果 |
|---------|------|------|
| `RemoveConst[T extends const?]` | 带 const | 剥离 const |
| `RemoveVolatile[T extends volatile?]` | 带 volatile | 剥离 volatile |
| `Pointer[T]` | 任意类型 | `*T` |
| `Slice[T]` | 任意类型 | `[]T` |
| `RemovePointer[T extends *?]` | 指针 | 解引用 |
| `RemoveSlice[T extends []?]` | 切片 | 解切片 |
| `ReturnType[F extends func]` | 函数类型 | 返回类型 |
| `SizeOf[T]` | 任意类型 | `u64`（编译期值） |

**内置变量**：

```c
builtin var VERSION: const str;     // 编译期常量
builtin var MAX_SIZE: u64;          // 编译期常量
```

**内置函数**：

```c
builtin func panic(msg: []u8): void;     // 编译器内联处理
builtin func sizeof(expr): u64;           // 编译期大小计算
builtin func alignof(expr): u64;          // 编译期对齐计算
```

类型变换指令直接作为类型表达式使用：

```c
type Mutable[T] = RemoveConst[T]
type Ptr[T] = Pointer[T]
type SlicePtr[T] = Slice[Pointer[T]]
```

容器相关的"元素类型"由容器自身通过 `type` 暴露（如 `Vec[i32].Element`），而非编译器内置。

#### 14. 泛型类型支持递归

```c
struct List[T] { head: T; tail: List[T] }
```

编译期模板展开，每次实例化时 T 已确定。

#### 15. struct 方法支持独立泛型参数

```c
struct Vec[T] {
    data: *T; len: u64
    type Element = T

    func[U](self: Vec[T], other: Vec[U]): Vec[T] { ... }
}
```

方法可带独立于 struct 的额外泛型参数。

#### 16. interface 支持泛型 + `type` 关联类型

```c
// 无泛型参数
interface Iterator {
    type Item
    func next(self: *Iterator): Item;
}

// 带泛型参数的 interface
interface Container[T] {
    func len(self: *Container[T]): u64;
    func get(self: *Container[T], idx: u64): T;
}

interface Mapper[K, V] {
    func map(self: *Mapper[K, V], key: K): V;
}
```

interface 支持泛型参数（在 `interface Name[T, ...]` 中声明），内部可定义方法签名和关联类型，关联类型在 implements 时被具体类型填充。与 struct/func 一致，泛型参数也在 `[]` 中声明。

匿名 interface 类型表达式（`interface [<generic_params>] { <members> }`）可作为类型表达式出现在泛型约束和类型位置，但不可作为真正的类型使用（无编译产物）。

## Coding Conventions

- C11 for library code, C++20 for tests
- Hand-crafted OOP via `type_t` virtual tables and struct nesting
- All memory managed through `allocator_t` — never use raw `malloc`/`free`
- `allocator_free(allocator, &ptr)` — always pass address of pointer; after call `ptr == NULL` (prevents use-after-free). **Do NOT manually set `ptr = NULL` after `allocator_free`** — the macro already handles this.
- **Error propagation in init functions**: When a subclass calls its parent's `type->init`, always wrap with `TRY_VOID_LOCAL(onerror, parent_type.init(&self->super, allocator, &super_init))`. This ensures that if the parent's init fails (sets `g_error`), the subclass's init jumps to `onerror:` for proper cleanup instead of continuing execution with partially-initialized state.
- **Dispose functions**: Do NOT assign `= NULL` after `allocator_free` calls — the macro already nullifies the pointer. Simply call `allocator_free` followed by the parent's `dispose`.
- Every data structure has a corresponding `g_xxx_type` global
- Error handling via `TRY`/`THROW`/`CATCH_ERROR` macros (Rust-like `?` pattern)
  - `TRY(ret, expr)` — Execute expr, return err if it throws (uses `return err` for propagation)
  - `TRY_LOCAL(label, expr)` — Execute expr, goto label on error (for cleanup paths with `goto onerror`)
  - `TRY_VOID_LOCAL(label, expr)` — Like `TRY_LOCAL` but for void expressions (init/dispose functions)
  - `THROW_LOCAL(label, fmt, ...)` — Throw error and goto label (used with `TRY_LOCAL` for cleanup)
  - `THROW(err, fmt, ...)` — Throw error and return err (used with `TRY` for direct return)
- Subclass init calls parent's `type->init` via `super` field
- Tests follow pattern: create allocator → test operations → destroy allocator (auto leak check)
- **Iterator-first for `list_t`**: Always use `list_iter_get`/`list_iter_set`/`list_iter_remove` for O(1) element access. Indexed `list_insert` is the only remaining O(n) index-based operation — used only for insertion position specification
- **Iterator for `vec_t`**: `vec_iter_first` creates iterator at index 0; `vec_iter_next` returns data then advances; `vec_iter_get`/`vec_iter_set` provide O(1) read/write at current position; `vec_iter_remove` deletes current element (O(n) shift), iterator stays at same index pointing to next element
