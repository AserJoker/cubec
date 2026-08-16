# Cubec 脚本执行后端

> 状态：**设计阶段**
>
> 背景文档：[engine-vm.md](../design/engine-vm.md)

---

## 1. 脚本执行后端概述

### 1.1 定义

**脚本执行后端** 是指将编译好的 Cubec 程序在运行时执行的功能模块，包括：
- AST → VM 指令翻译（字节码生成或解释器）
- VM 运行时执行引擎
- 类型系统运行时支持
- 函数调用与控制流执行

### 1.2 架构关系

```
┌─────────────────────────────────────────────────────┐
│           编译器前端（已完成）                         │
│  lexer → parser → checker → comptime_eval            │
└───────────────────────┬─────────────────────────────┘
                        │ 输出：AST
                        │
                        ▼
┌─────────────────────────────────────────────────────┐
│       脚本执行后端（Engine VM，部分完成）             │
│  ┌──────────────┐  ┌──────────────┐                │
│  │ 类型系统     │  │ 值操作引擎   │                │
│  │ (已完成)     │  │ (已完成)     │                │
│  └──────────────┘  └──────────────┘                │
│  ┌──────────────┐  ┌──────────────┐                │
│  │ 作用域管理   │  │ 模块加载     │                │
│  │ (已完成)     │  │ (已完成)     │                │
│  └──────────────┘  └──────────────┘                │
│  ┌──────────────────────────────────────┐          │
│  │   缺失：VM 运行时执行接口             │          │
│  │   - AST→指令翻译器                    │          │
│  │   - 函数调用引擎                       │          │
│  │   - 控制流执行                         │          │
│  └──────────────────────────────────────┘          │
└───────────────────────┬─────────────────────────────┘
                        │
                        ▼
              ┌────────────────┐
              │  可执行程序     │
              │  (由 C 后端生成) │
              └────────────────┘
```

---

## 2. Engine VM 已实现功能

### 2.1 类型系统运行时（约 5,200 行）

| 模块 | 代码量 | 功能 |
|------|--------|------|
| `type.c` | 182 | 类型抽象、类型哈希与缓存 |
| `bool_type.c` | 203 | `bool` 类型实现 |
| `void_type.c` | 70 | `void` 类型实现 |
| `wildcard_type.c` | 14 | `<?>` 通配符类型 |
| `integer_type.c` | 653 | `i8/i16/i32/i64/u8/u16/u32/u64` |
| `float_type.c` | 469 | `f16/f32/f64` |
| `result_type.c` | 248 | `Result<T, E>` 类型 |
| `pointer_type.c` | 541 | 指针类型（`*T`、`*const T`、`*volatile T`） |
| `array_type.c` | 436 | 数组类型（`[N]T`） |
| `slice_type.c` | 455 | 切片类型（`[]T`） |
| `tuple_type.c` | 523 | 元组类型（`<T1, T2>`） |
| `callable_type.c` | 472 | 可调用类型（函数类型） |
| `struct_type.c` | 1077 | 结构体类型（含 vtable、魔术方法） |
| `union_type.c` | 1175 | 联合类型（tagged/cunion） |
| `interface_type.c` | 9805 | 接口类型（类接口） |

**统计**：类型系统已实现 **9,000+ 行**代码。

### 2.2 值操作引擎（437 行）

`src/engine/value.c` 提供统一的值表示：

```c
typedef struct _value_t {
  type_t type;         /* 类型对象指针 */
  void  *data;         /* 原始数据缓冲（shadow=NULL） */
  void  *meta;         /* 元数据（类对象、魔术方法） */
  bool   own;          /* 是否拥有 data */
  bool   initialized;  /* 是否已初始化（TDZ 检查） */
} value_t;
```

**关键接口**：
- `value_create()` - 创建值
- `value_create_ref()` - 创建引用
- `value_create_shadow()` - 创建 shadow 值（编译期用）
- `type_get_size()` / `type_get_align()` - 获取类型信息

### 2.3 作用域管理（322 行）

`src/engine/scope.c` 提供层级作用域树：

```c
typedef struct _scope_t {
  scope_t parent;      /* 父作用域 */
  enum scope_kind kind; /* GLOBAL / MODULE / FUNCTION / BLOCK */
  strmap_t names;      /* name_t (名称 → 值引用) */
  vec_t values;        /* value_t (所有值，auto_dispose) */
  vec_t types;         /* type_t (所有类型，auto_dispose) */
  vec_t strings;       /* 字符串缓存 */
  vec_t cfuncs;        /* C 函数绑定 */
  vec_t defers;        /* defer 清理栈 */
  scope_t owner;       /* 所属模块或函数 */
} scope_t;
```

**功能**：
- `scope_create()` - 创建作用域
- `scope_lookup()` - 作用域变量查找（向上遍历）
- `scope_add_child()` / `scope_remove_child()` - 子作用域管理

### 2.4 模块加载系统（373 行）

`src/engine/module.c` 提供：

```c
typedef struct _module_t {
  allocator_t allocator;
  const char *filename;    /* 文件名 */
  const char *source;      /* 源码（borrowed） */
  vec_t tokens;            /* 词法结果 */
  node_t program;          /* AST */
  scope_t root_scope;      /* 模块作用域 */
  strmap_t exports;        /* 导出符号表 */
  enum module_state state; /* NEW / PARSED / CHECKED / ERROR */
} module_t;
```

**接口**：
- `module_create()` - 创建模块
- `module_dispose()` - 销毁模块

### 2.5 命名收集系统（368 行）

`src/engine/name_collector.c` 用于泛型单态化：

```c
/**
 * @brief Run name collection on a module.
 *
 * Traverses the module's program AST, registering declaration names into the
 * module's root_scope.
 */
void name_collector_run(context_t ctx, module_t mod);
```

**功能**：
- 遍历 AST 收集声明名称
- 处理 import 语句（递归加载依赖模块）
- 注册导出符号（`export var X`）

### 2.6 VM 基础架构（765 行）

`src/engine/vm.c` 提供核心运行时：

```c
typedef struct _vm_t {
  allocator_t allocator;
  strmap_t modules;      /* 绝对路径 → module_t */
  scope_t global_scope;  /* 全局作用域（owned） */
  scope_t root_scope;    /* 当前模块根作用域（borrowed） */
  scope_t current_scope; /* 当前遍历位置（borrowed） */

  /* 内置类型（borrowed，全局唯一） */
  value_t v_type;
  value_t v_exception;
  value_t v_bool;
  value_t v_wildcard;
  value_t v_void;
  value_t v_i8, v_i16, v_i32, v_i64;
  value_t v_u8, v_u16, v_u32, v_u64;
  value_t v_f16, v_f32, v_f64;
  value_t v_str;
  value_t v_error;
  value_t v_wildcard_tuple;
  value_t v_wildcard_value;

  const char *current_module_id;  /* 当前模块 ID */
  vec_t call_stack;                /* 调用栈 */
} vm_t;
```

**接口**：
- `vm_create()` / `vm_dispose()` - 生命周期
- `vm_get_module()` / `vm_import()` - 模块加载
- `vm_push_scope()` / `vm_pop_scope()` - 作用域切换
- `vm_push_frame()` / `vm_pop_frame()` - 调用栈管理
- `vm_create_value()` - 创建值
- 类型实例化接口（`vm_create_*_type_value`）

### 2.7 编译上下文（44 行）

`src/engine/context.c` 提供：

```c
typedef struct context {
  allocator_t allocator;
  vm_t vm;                   /* owned: VM 实例 */
  diagnostic_list_t diagnostics;  /* 诊断系统 */
} context_t;
```

**接口**：
- `context_create()` / `context_dispose()` - 生命周期
- `context_get_error_count()` - 错误统计

---

## 3. 缺失功能

### 3.1 VM 运行时执行接口

当前 VM **没有**以下关键接口：

| 功能 | 预期接口 | 状态 |
|------|----------|------|
| 模块执行 | `vm_run(module_t)` | ❌ 缺失 |
| 程序入口 | `context_run(context_t, program_t)` | ❌ 缺失 |
| 函数调用执行 | `vm_call_function(value_t fn, ...)` | ❌ 缺失 |
| 表达式求值 | `vm_eval_expression(node_t expr)` | ❌ 缺失 |
| 语句执行 | `vm_execute_statement(node_t stmt)` | ❌ 缺失 |
| 变量赋值执行 | `vm_assign(name_t name, value_t val)` | ❌ 缺失 |
| 控制流执行 | `vm_execute_control_flow(node_t cf)` | ❌ 缺失 |
| defer 执行 | `vm_execute_defer()` | ❌ 缺失 |

### 3.2 AST → 指令翻译器

当前缺少将 AST 节点翻译为 VM 可执行指令的组件：

- **字节码生成器**：将 AST 节点翻译为字节码指令
- **解释器**：直接解释执行 AST 节点

可选方案：
- **字节码 VM**：编译为字节码 → 解释器执行（性能可控）
- **直接解释器**：直接解释执行 AST（开发效率高）
- **混合模式**：comptime 部分编译期求值，运行时解释执行

### 3.3 函数调用引擎

需要实现：
- 参数压栈/解包
- 返回值处理
- 闭包捕获环境
- 作用域切换

### 3.4 控制流执行

需要实现：
- if/else 执行
- for/while 循环
- switch/match
- defer 执行顺序（LIFO）

---

## 4. 实现策略

### 4.1 阶段一：基础执行引擎（解释器）

**目标**：实现最小可运行程序（"Hello World"）

**功能**：
1. 实现 `vm_execute_statement()` - 解释执行单个语句
2. 实现 `vm_execute_function()` - 解释执行函数体
3. 实现变量赋值与访问
4. 实现基础表达式求值
5. 实现 main 函数入口

**示例**：
```c
context_t ctx = context_create(allocator);
module_t mod = vm_import(ctx->vm, ctx, "main.cubec");
vm_run(ctx->vm, mod);
context_dispose(ctx);
```

### 4.2 阶段二：完整控制流

**目标**：支持完整控制流语句

**功能**：
- if/else
- for/while
- switch
- defer

### 4.3 阶段三：性能优化

**目标**：提高运行时性能

**方案**：
- 选择字节码 VM 或直接解释器
- 实现即时编译（JIT）入口
- 优化热路径

---

## 5. 与 Comptime 的关系

### 5.1 Comptime 执行

**现状**：Comptime 求值器已实现（`comptime_eval.c`），在编译期执行。

**作用**：
- 测试块执行（`test {...}`）
- 编译期条件分支（`comptime if`）
- 编译期迭代（`comptime foreach`）

### 5.2 运行时与 Comptime 的区别

| 特性 | Comptime | 运行时 |
|------|----------|--------|
| 执行时机 | 编译期 | 运行期 |
| 数据来源 | 静态 AST | 运行时值 |
| 内存 | 虚拟内存 | 真实内存 |
| SEH 保护 | ✅ | ❌（或可选） |
| 测试块 | ✅ | ❌ |
| 可见性 | 影响编译结果 | 不影响编译 |

### 5.3 共享组件

**Engine VM 同时支持两种模式**：
- **Shadow Engine**：编译期用（data=NULL，只检查）
- **真实 VM**：运行期用（data!=NULL，真实执行）

**优势**：
- 一套代码服务两种场景
- Engine 不需要编译特化逻辑
- 类型系统运行时可以复用

---

## 6. 数据流示例

### 6.1 编译期（Comptime）

```
源码: comptime var N = 10;
  ↓
词法分析
  ↓
语法分析
  ↓
comptime_eval:
  - 创建 shadow value (type=i32, data=NULL, initialized=false)
  - 求值表达式 10 → value (type=i32, data=10, initialized=true)
  - 保存到作用域 (global_scope->values)
  ↓
语义检查通过
  ↓
生成 C 代码（comptime 计算完毕，不输出 C 代码）
```

### 6.2 运行期（未来）

```
源码: var x = 10;
  ↓
词法分析
  ↓
语法分析
  ↓
语义检查（使用 Shadow Engine）
  ↓
代码生成（C 后端）:
  - 生成 C 变量声明: `int x = 10;`
  ↓
C 编译器:
  - 生成机器码
  ↓
运行期执行:
  - 加载可执行文件
  - main 函数执行，x=10
```

**问题**：当前 C 后端已可用，但缺少直接运行 cubec 脚本的能力。

---

## 7. 下一步计划

### 7.1 短期（1-2 周）

1. **设计 VM 运行时接口**
   - 定义 `vm_run()`、`vm_execute_statement()` 等接口
   - 确定解释器或字节码方案

2. **实现基础解释器**
   - AST → 解释执行
   - 变量赋值与访问
   - 简单表达式求值

3. **实现 main 函数入口**
   - `context_run()` 入口
   - 调用 main 函数
   - 返回值处理

### 7.2 中期（1-2 月）

1. **完整控制流支持**
   - if/else
   - for/while
   - switch
   - defer

2. **模块执行**
   - 多模块加载
   - 循环依赖处理
   - 导入/导出

3. **错误处理**
   - `.?` 错误传播
   - `panic` 处理
   - 异常捕获（如需要）

### 7.3 长期（3+ 月）

1. **性能优化**
   - 选择最佳 VM 方案
   - JIT 入口（可选）

2. **标准库支持**
   - 实现基础库函数
   - 文件 I/O
   - 网络

3. **调试支持**
   - 运行时栈追踪
   - 断点调试

---

## 8. 参考文档

- [engine-vm.md](../design/engine-vm.md) - Engine VM 设计规格
- [01-architecture.md](01-architecture.md) - 核心架构（TDZ、多遍扫描）
- [comptime.md](07-comptime.md) - Comptime 求值器（待创建）
- [13-ast-semantics.md](13-ast-semantics.md) - AST 节点语义

---

**总结**：Engine VM 已实现类型系统、作用域管理、模块加载、命名收集等核心组件（约 9,000+ 行代码），但缺少运行时执行接口和 AST→指令翻译器。下一步是实现基础解释器，支持最小可运行程序。
