# Cubec AST 节点语义与引擎模块结构

> 本文档基于 `src/engine/`、`src/cubec/`、`include/engine/`、`include/cubec/` 的
> 实际实现整理。早期设计文档中提到的 `semantic_type`、`resolver`、`flow`、
> `comptime_eval`、`comptime_alloc`、`module_cache`、`builtin`、
> `checker_desugar_*` 等模块**均已不存在**，已被统一的 VM 引擎取代。

## 1. 引擎（engine）模块结构

### 1.1 文件职责

```
src/engine/
├── context.c/h          # 编译上下文：context_create / dispose / get_error_count
├── vm.c/h               # 引擎核心：模块导入、作用域切换、调用栈、内置类型、类型构造
├── module.c/h           # 模块对象：module_create / dispose，含 module_state
├── name_collector.c/h   # 名字收集：注册声明名字、递归处理 import
├── scope.c/h            # 作用域树：scope_create / lookup / add_child
├── name.c/h             # 名字绑定：name_t 持有 borrowed value_t
├── diagnostic.c/h       # Rustc 风格诊断系统
├── source.c/h           # 源码缓存（按文件名缓存，按行提取）
├── value.c/h            # 统一值表示与值层操作（运算/字段/下标/调用/解引用）
├── type.c/h             # 类型抽象、类型哈希与缓存
├── integer_type.c       # i8/i16/i32/i64/u8/u16/u32/u64 实现
├── float_type.c         # f16/f32/f64 实现
├── bool_type.c          # bool 实现
├── void_type.c          # void 实现
├── wildcard_type.c      # ? 通配符类型实现
├── pointer_type.c       # *T / *const T / *volatile T 实现
├── array_type.c         # [N]T 实现
├── slice_type.c         # []T 实现
├── tuple_type.c         # (T1, T2, ...) 实现
├── struct_type.c        # struct 实现（含 vtable、魔法方法）
├── union_type.c         # union（tagged）/ cunion 实现
├── enum_type 相关       # enum 实现（见 engine 目录）
├── interface_type.c     # interface 约束检查实现
├── callable_type.c      # 函数类型实现
├── exception_type.c     # 异常类型实现
├── str_type.c           # 字符串类型实现
└── result_type.c        # Result[T,E] 类型实现
```

### 1.2 核心结构

```c
/* 编译上下文 */
struct context {
    allocator_t allocator;
    vm_t vm;                   /* owned: VM 实例 */
    diagnostic_list_t diagnostics;
};

/* 模块 */
struct _module_t {
    allocator_t allocator;
    const char *filename;  /* owned */
    char *source;          /* owned source text */
    vec_t tokens;          /* owned token list */
    node_t program;        /* owned AST root node */
    scope_t root_scope;    /* owned: module root scope */
    strmap_t exports;      /* exported names: symbol name → name_t */
    int state;             /* enum module_state */
};

/* 模块处理状态 */
enum module_state {
    MODULE_NEW,        /* 未处理 */
    MODULE_COLLECTING, /* 名字收集中 */
    MODULE_COLLECTED,  /* 名字收集完成 */
    MODULE_RESOLVING,  /* 定义解析中 */
    MODULE_RESOLVED,   /* 定义解析完成 */
    MODULE_CHECKED,    /* 类型检查完成 */
};

/* 作用域 */
struct _scope_t {
    allocator_t allocator;
    enum scope_kind kind;
    struct _scope_t *parent;
    vec_t children;
    strmap_t names;   /* text → name_t (owned) */
    vec_t values;     /* value_t (auto-dispose, owns) */
    vec_t types;      /* type_t (auto-dispose, owns) */
    vec_t strings;
    vec_t cfuncs;
    vec_t defers;
    void *owner;
};

/* 名字绑定 */
struct _name_t {
    allocator_t allocator;
    value_t ref; /* borrowing: points to the value in scope->values */
};

/* 值对象（opaque，见 engine/value.h） */
struct _value_t;
typedef struct _value_t *value_t;

/* 类型对象 */
struct _type_t {
    type_kind_t kind;
    char *name;
    uint64_t size;
    uint64_t align;
    bool mut;       /* true = 可变，false = 只读（const） */
    vtable_t vtable;
};
```

---

## 2. AST 节点语义确认

### 2.1 逗号表达式（EXPRESSION_COMMA）

C 风格逗号表达式：从左到右求值，返回最右值。

```c
// Cubec: a, b, c  → 求值 a, b, c，整个表达式值为 c
```

- 优先级最低（低于赋值）
- 右结合：`a, b, c` → `comma(a, comma(b, c))`
- 仅用于 for 循环初始化/更新等特殊场景
- 逗号在函数参数、初始化列表中仅作分隔符，不构成逗号表达式

### 2.2 取地址运算符（EXPRESSION_ADDR）

语法：后缀 `.&`，操作数**必须是左值**（经 `value_addrof` 实现）。

```c
var x: i32 = 42;
var p: *i32 = x.&;       // &x
var p: *i32 = (x + 1).&; // 非法：临时值不可取地址
```

### 2.3 成员调用分派

实例方法调用经 `value_member_call` 分派：

```c
a.method(arg1, arg2)
→ typeof(a)::method(&a, arg1, arg2)   // 编译器取 &a 作首参
```

- 指针 auto-deref：如果 `a` 是指针类型，经 vtable 自动解引用后查找方法
- `self` 接收规则：方法第一个参数 `self` 对应 `&a` 的类型

### 2.4 匿名函数与闭包

```c
func |capture_list| [generic_params] (params) [: return_type] { body }
```

- 捕获列表：标识符 only，无表达式初始化
- 按值捕获（值层 deep copy 到隔离环境）
- 空捕获列表：`||`（lexer 将 `||` tokenize 为单 token，需特殊处理）

### 2.5 展开运算符 `...`

三种用途（与 `04-expressions.md` / `08-generics.md` 一致）：

1. **类型复用**：struct/union/cunion 中的 `...Type` 展开字段和方法
2. **泛型参数包**：`...T` 定义变参泛型
3. **调用/初始化展开**：函数调用 `fn(...args)`、初始化列表 `Type{...base}`

### 2.6 const/volatile 限定符

- 合并为 `declaration_qualifier` 节点，由 `type_t.mut` 标志表达 const
- 指针 const 映射：`*const T` → 指向 const，`const *T` → const 指针
- Const 传播：成员访问、解引用、赋值的 const 检查经 `vtable.assignment`
- comptime 只强制 const，忽略 volatile

### 2.7 魔法方法

| 方法 | 用途 | 分派 vtable |
|------|------|-----------|
| `__get__` | 下标读取 `a[i]` | `get_item` |
| `__set__` | 下标写入 `a[i] = v` | `set_item` |
| `__value__` | 自动拆箱/隐式转换 | `safe_cast` |
| `__call__` | 函数调用 `a()` | `call` |
| `__slice__` | 切片 `a[s:l]` | `slice` |

- `__get__`/`__set__` 拦截 `[]`，不拦截 `.`
- `__value__` 类似 JS 的 `Symbol.toPrimitive`

### 2.8 Decorator（新增特性）

Cubec 支持 C++11 属性风格的 decorator 语法 `[[expression]]`，可附加在声明上
（struct/union/enum/interface/func/type alias/cunion 均含 `decorators` 向量）：

```c
[[deprecated("use foo2 instead")]]
func foo(): void { ... }

[[inline]]
func bar(): i32 { return 1; }
```

- 解析器 `read_decorator` 识别 `[[` ... `]]`，内部表达式经
  `read_expression_call` 支持 `keyword(args)` 形式
- decorator 作为 AST 节点（`cubec_decorator_t`）保留，供 codegen / 工具消费
- 这是早期文档未包含的语法特性
