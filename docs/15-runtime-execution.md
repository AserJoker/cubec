# Cubec 引擎（VM）实现状态

> 基于 `src/engine/`、`include/engine/` 的实际代码整理。本文档取代早期
> "脚本执行后端" 设计稿——早期稿将引擎描述为"类型系统已完成、运行时执行接口缺失"，
> 但当前代码已经是一个统一的、可用于 comptime 求值与语义检查的值/类型引擎（VM）。

---

## 1. 引擎概述

Cubec 编译器围绕一个统一的**值/类型引擎（VM）**构建。前端（lexer → parser）
产出 AST 后，语义检查与 comptime 求值都通过 VM 完成：

```
┌─────────────────────────────────────────────────────┐
│           编译器前端（已完成）                          │
│  lexer → parser → AST                                │
└───────────────────────┬─────────────────────────────┘
                        │ 输出：AST
                        ▼
┌─────────────────────────────────────────────────────┐
│       引擎 VM（已实现，src/engine/）                  │
│  ┌──────────────┐  ┌──────────────┐                 │
│  │ 类型系统     │  │ 值操作引擎   │                 │
│  │ (type*.c)   │  │ (value.c)    │                 │
│  └──────────────┘  └──────────────┘                 │
│  ┌──────────────┐  ┌──────────────┐                 │
│  │ 作用域管理   │  │ 模块加载     │                 │
│  │ (scope.c)   │  │ (module.c)   │                 │
│  └──────────────┘  └──────────────┘                 │
│  ┌──────────────────────────────────────┐           │
│  │  名字收集 (name_collector.c)          │           │
│  │  统一管线驱动                          │           │
│  └──────────────────────────────────────┘           │
└───────────────────────┬─────────────────────────────┘
                        │
                        ▼
              ┌────────────────┐
              │  C 后端代码生成 │
              │  (src/cubec/*)  │
              └────────────────┘
```

---

## 2. 已实现功能

### 2.1 类型系统

每种类型是一个带 `vtable_t` 的对象，定义在 `src/engine/<kind>_type.c`：

| 模块 | 文件 | 类型 |
|------|------|------|
| `type.c` | 类型抽象、类型哈希与缓存、bootstrap |
| `bool_type.c` | `bool` 类型 |
| `void_type.c` | `void` 类型 |
| `wildcard_type.c` | `?` 通配符类型 |
| `integer_type.c` | `i8/i16/i32/i64/u8/u16/u32/u64` |
| `float_type.c` | `f16/f32/f64` |
| `pointer_type.c` | 指针类型（`*T`/`*const T`/`*volatile T`） |
| `array_type.c` | 数组类型（`[N]T`） |
| `slice_type.c` | 切片类型（`[]T`） |
| `tuple_type.c` | 元组类型 `(T1, T2)` |
| `callable_type.c` | 可调用类型（函数类型） |
| `struct_type.c` | 结构体类型（含 vtable、魔法方法） |
| `union_type.c` | 联合类型（tagged/cunion） |
| `interface_type.c` | interface 约束检查 |
| `exception_type.c` | 异常类型 |
| `str_type.c` | 字符串类型 |
| `result_type.c` | `Result[T, E]` 类型 |

所有类型操作（相等、转换、字段访问、运算、调用、解引用、切片等）通过
`vtable_t` 分派，见 `02-type-system.md` 第 1.2 节。

### 2.2 值操作引擎（value.c）

`value_t` 是统一的值表示（opaque），所有操作经 `engine/value.h` 接口分派到
类型 vtable：

- 创建：`value_create` / `value_create_ref` / `value_create_shadow`（TDZ 占位）
- 相等/克隆：`value_equal` / `value_clone`
- 运算：`value_add`/`sub`/`mul`/`div`/`mod`/`band`/`bor`/`bxor`/`shl`/`shr`/`gt`/`lt`/...
- 转换：`value_safe_cast`
- 赋值：`value_assignment`（检查 TDZ/const，拷贝 data，置 `initialized=true`）
- 字段：`value_get_field` / `value_set_field` / `value_member_call`
- 下标：`value_get_item` / `value_set_item`
- 解引用：`value_deref_get` / `value_deref_set`
- 切片：`value_slice`
- 调用：`value_call`
- 地址：`value_addrof` / `value_member_addr`
- 类型检查：`value_is`（union `is` 运算符）
- 静态属性：`value_get_prop` / `value_set_prop`

### 2.3 作用域管理（scope.c）

层级作用域树，结构见 `01-architecture.md` 第 4.2 节。

### 2.4 模块加载系统（module.c）

`module_t` 持有 filename/source/tokens/program/root_scope/exports/state。
`vm_import` 负责：解析路径 → 读文件 → 词法 → 解析 AST → 创建模块，结果缓存
（重复 import 返回已有模块）。

### 2.5 名字收集系统（name_collector.c）

`name_collector_run(ctx, mod)` 遍历 AST 注册声明名字到 `root_scope`，
递归处理 `import`，注册导出符号。这是统一管线的驱动入口，逐作用域递归。

### 2.6 VM 核心（vm.c）

`vm_t` 持有：

- `modules`（strmap：绝对路径 → module_t）
- `global_scope` / `root_scope` / `current_scope`
- 内置类型值（`v_i32`、`v_str`、`v_void`、`v_bool`、`v_wildcard`、各数值类型 const 变体等）
- `current_module_id`（用于访问控制：私有字段仅同模块可访问）
- `call_stack`（vec of `call_frame_t`）

接口：`vm_import`、`vm_push_scope`/`vm_pop_scope`、`vm_push_frame`/`vm_pop_frame`、
`vm_create_value*` 系列（构造各复合类型并注册到当前作用域）。

### 2.7 编译上下文（context.c）

```c
struct context {
    allocator_t allocator;
    vm_t vm;                   /* owned */
    diagnostic_list_t diagnostics;
};
```

---

## 3. 与 Comptime 的关系

### 3.1 执行模型

VM 同时支持两种模式：

- **编译期（comptime）**：类型检查、comptime var/func 求值、test 块、build 脚本，
  经 VM 值层执行（shadow value 仅检查、不占真实内存）
- **运行期产物**：经 C 后端（`src/cubec/`）生成 C 代码，由 C 编译器产出可执行文件

### 3.2 共享组件

类型系统、值操作、作用域、模块加载为编译期与运行期（代码生成）共用，
避免为语义检查单独维护一套类型表示。

---

## 4. 数据流示例

### 4.1 编译期（comptime）

```
源码: comptime var N = 10;
  ↓ 词法 / 语法
  ↓ name_collector_run → 注册 N 名字
  ↓ VM 求值表达式 10 → value(type=i32, data=10, initialized=true)
  ↓ 保存到作用域 (current_scope->values)
  ↓ 后续声明可见 N
生成 C 代码（comptime 计算完毕，不输出 C 代码）
```

### 4.2 运行期（代码生成）

```
源码: var x = 10;
  ↓ 词法 / 语法 / 名字收集 / VM 语义检查
  ↓ C 后端生成: int x = 10;
  ↓ C 编译器生成机器码
  ↓ 运行期 x = 10
```

---

## 5. 待实现 / 后续事项

- **LLVM 后端**：当前仅有 C 后端（`src/cubec/`），LLVM 后端尚未开始
- **完整依赖管理 CLI**：`cubec.json` / `fetch` / 项目模式在代码中尚未实现
  （当前 CLI 仅有 `format` 子命令，见 `14-dependency-module.md`）
- **标准库**：`std/*` 模块需在依赖系统就绪后提供
