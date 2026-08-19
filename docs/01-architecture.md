# Cubec 核心架构

> 本文档基于 `src/engine/` 与 `src/cubec/` 的实际实现整理。编译器前端用 C11 编写，
> 采用"手写 C 风格 OOP"模式（每个数据结构有对应的 `g_xxx_class` 虚表，通过
> `allocator_create` 实例化）。所有内存通过 `allocator_t` 管理。

## 1. 核心原则

### 1.1 错误恢复

语义检查不因单点失败而中止，跳过当前节点继续处理兄弟节点，收集所有错误。

- 失败节点返回 error value（占位），防止级联报错
- 诊断收集到列表，遍历不中断
- 错误去重，避免同一错误重复报告

### 1.2 Rustc 风格诊断

```
error: type mismatch
  --> test.cubec:3:14
   |
 3 | var x: i32 = "hello"
   |              ^^^^^^^ expected i32, found string
```

- severity（error/warning/note）
- 主位置 + 附注位置
- 源码缓存按文件名缓存，按行号提取
- 行号对齐、`|` 标尺、`^` 下划线标注 span

诊断由 `engine/diagnostic.c` 实现，与源码缓存（`engine/source.c`）配合。

### 1.3 显式分配器

所有内存操作走显式 `allocator_t` 参数（Zig 风格），使同一函数可在编译期（VM 值层）和运行期（真实分配器）执行。

| 分配器 | 场景 | 内存 |
|--------|------|------|
| `heap_allocator` | 编译器运行时 | 真实堆内存 |
| VM 值层 | comptime / 语义检查 | VM 内部值对象 |
| `arena_allocator` | 编译器临时结构 | 真实但可批量释放 |

---

## 2. 统一引擎（VM）架构

Cubec 编译器不是"lexer → parser → checker → codegen"的离散阶段加一堆独立模块，
而是围绕一个**统一的值/类型引擎（VM）**构建。核心思想：

- **类型是一等的值对象**：每个类型 `type_t` 都是一个对象，带 `vtable_t` 行为分发表
- **值是一等的值对象**：`value_t` 统一表示编译期常量、变量、类型、函数等
- **符号即值**：作用域的 `names` 表把名字映射到 `value_t`（borrowed 指针）
- **编译期与运行期共用一套类型/值操作**：通过 vtable 分派，避免两套代码

### 2.1 核心数据结构

```
context_t          — 编译顶层上下文（持有 vm + diagnostics）
  └─ vm_t          — 引擎核心（类型、作用域、模块、调用栈）
       ├─ scope_t global_scope / root_scope / current_scope
       ├─ module_t（strmap: 绝对路径 → module）
       └─ 内置类型值（v_i32, v_str, v_void, ...）
  └─ diagnostic_list_t  — 诊断收集
```

关键模块（`include/engine/` + `src/engine/`）：

| 模块 | 职责 |
|------|------|
| `context.c` | 编译上下文：`context_create` / `context_dispose` / `context_get_error_count` |
| `vm.c` | 引擎核心：模块导入、作用域切换、调用栈、内置类型、类型构造 |
| `module.c` | 模块对象：`module_create` / `module_dispose`，含 `module_state` |
| `name_collector.c` | 名字收集：把 AST 声明名字注册进作用域，递归处理 import |
| `scope.c` | 作用域树：`scope_create` / `scope_lookup` / `scope_add_child` |
| `name.c` | 名字绑定：`name_t` 持有一个 borrowed `value_t` |
| `value.c` | 统一值表示与值层操作（运算、字段访问、调用、解引用等） |
| `type.c` | 类型抽象、类型哈希与缓存 |
| `integer_type.c` `float_type.c` `bool_type.c` `void_type.c` `pointer_type.c` | 各类型实现，各自定义 vtable |
| `array_type.c` `slice_type.c` `tuple_type.c` `struct_type.c` `union_type.c` | 复合类型实现 |
| `callable_type.c` `interface_type.c` `exception_type.c` `wildcard_type.c` `str_type.c` `result_type.c` | 其它类型实现 |
| `diagnostic.c` `source.c` | 诊断与源码缓存 |

> 注意：早期设计文档提到的 `semantic_type`、`type_impl`、`resolver`、
> `resolver_types`、`checker`、`inference`、`flow`、`comptime_eval`、
> `comptime_alloc`、`module_cache`、`builtin`、`checker_desugar_*` 等模块
> **在当前代码中均已不存在**，已被上述统一 VM 引擎取代。

### 2.2 编译流程

因 comptime 表达式存在，AST 需要多遍处理。流程由 `name_collector` 驱动，
逐作用域递归：

**第一遍：名字收集（建作用域）**

`name_collector_run` 遍历 program AST，只注册名字，不展开 body：

- struct/enum/union/cunion/interface/type → 注册 TYPE 名字
- func → 注册 FUNCTION 名字
- var → 注册 VARIABLE 名字
- import → 触发依赖模块的名字收集，注册其导出名字
- 类型声明（struct/union/interface）→ 先注册类型名，再递归进入类型作用域收集成员

**第二遍及以后：按序定义解析 / 检查 / 实例化**

每个作用域内逐层执行：定义解析 → 泛型实例化 → 函数体解析，再递归进入子作用域。
非静态作用域（函数体、块、for/foreach）中变量**不提升**（按文本顺序可见），
但函数和类型仍提升。

> 与早期"四遍扫描"（声明收集 → 按序求值 → 函数体检查 → 泛型实例化）描述相比，
> 当前实现改为**逐作用域递归**的统一管线，局部作用域内也可有函数/类型声明。

### 2.3 全局 vs 函数体内 comptime

两个阶段语义一致：

**全局阶段** — 影响后续声明的可见性：

```c
comptime var N = 10;        // 求值后 N 可用
comptime if (N > 5) {       // 依赖 N
    export var big = true;
}
```

**函数体内阶段** — 全局已定型，只影响当前函数：

```c
func foo[T](x: T) {
    comptime if (T extends Numeric) {   // T 在实例化时确定
        var doubled = x * 2;
    }
}
```

---

## 3. 双层语义已重构为值/类型引擎

早期设计将符号分为三阶段（TDZ → Evaluated）、类型分为两级（NameKnown →
DefinitionKnown）、并有独立的 `semantic_type_t` 双层结构。当前代码统一为：

- **符号 = 值**：`name_t` 直接指向一个 `value_t`
- **TDZ 由 value 的 `initialized` 标志表达**：`value_create_shadow(type, name, false)` 创建 TDZ 占位值，`initialized=true` 后离开 TDZ
- **类型两级**由 `type_t` 的 `kind`（如 `TYPE_KIND_STRUCT`）在定义解析前后是否计算 `size`/`align` 体现，而非独立的 impl 层
- **`undefined`** 创建 TDZ 占位值：`var x: i32 = undefined` → `initialized=false`，显式赋值后 `initialized=true`

```c
var x: i32 = undefined;   // value_t: type=i32, initialized=false (TDZ)
x = 42;                   // value_assignment → initialized=true
```

- `var` 语句在非 builtin/extern 场景下**必须有初始化器**
- `= undefined` 是合法初始化器，但变量处于 TDZ 直到显式赋值
- 消除"可能未赋值"问题，简化控制流分析

---

## 4. 作用域与符号表

### 4.1 作用域种类

作用域种类（`enum scope_kind`，定义于 `include/engine/scope.h`）：

```c
enum scope_kind {
    SCOPE_GLOBAL,        // 全局/模块顶层
    SCOPE_MODULE,        // 模块作用域
    SCOPE_FUNCTION,      // 参数 + 局部变量
    SCOPE_BLOCK,         // {} 内的变量
    SCOPE_FOR,           // for init 变量
    SCOPE_FOREACH,       // foreach 迭代变量
    SCOPE_TYPE,          // struct/enum/union/cunion/interface 的成员
};
```

> 注意：早期文档描述的 `SCOPE_PROGRAM` / `SCOPE_COMPTIME` /
> `SCOPE_TYPE_INSTANCE` / `SCOPE_TYPE_STATIC` 在当前代码中已不存在；
> 类型成员统一在 `SCOPE_TYPE` 内，instance/static 区分由方法签名（是否带 `self`）决定。

### 4.2 作用域结构

```c
struct _scope_t {
    allocator_t allocator;
    enum scope_kind kind;
    struct _scope_t *parent; /* parent scope */
    vec_t children;          /* child scopes (auto-dispose vec) */
    strmap_t names;          /* name table: text → name_t (owned) */
    vec_t values;            /* all values in this scope (auto-dispose, owns value_t) */
    vec_t types;             /* dynamic types created in this scope (auto-dispose, owns type_t) */
    vec_t strings;           /* string_t objects in this scope (auto-dispose) */
    vec_t cfuncs;            /* func_t objects in this scope (auto-dispose) */
    vec_t defers;            /* defer entries (empty for now) */
    void *owner;             /* borrowing pointer to owning object (module/function) */
};
```

### 4.3 名称与符号

```c
struct _name_t {
    allocator_t allocator;
    value_t ref; /* borrowing: points to the value in scope->values */
};
```

名称解析即 `scope_lookup(scope, name)`：从内向外逐层查找 `names` 表，命中返回
`name_t`，其 `ref` 指向作用域 `values` 中的实际值对象。

- **简单标识符 `x`** — `scope_lookup` 由内向外查找
- **`.` 成员访问 `expr.field`** — 解析左侧类型，经 `value_get_field` / `value_member_call` 分派到类型 vtable
- **`::` 命名空间访问 `Type::member`** — 经 `value_get_prop` 分派到类型级 vtable

### 4.4 self 与方法调用

- `self` 始终是指针（`*StructType`）
- member call 是语法糖：`obj.method(args)` → 编译器取 `&obj` 作为首参调用 `typeof(obj)::method`
- 方法查找通过类型 vtable 的 `member_call` 实现，按当前指针类型的方法表分派
