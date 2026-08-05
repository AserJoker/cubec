# 名字收集阶段设计文档

## 1. 概述

名字收集是编译管线的第一个语义阶段，负责将当前作用域内的声明名字注册到作用域的 name 表中，使名字在定义前即可被引用（提升）。这是后续定义解析、实例化、函数体解析的基础。

## 2. 核心设计原则

### 2.1 统一 template → instances

所有函数/类型统一为 template → instances 结构，不区分泛型与非泛型。无泛型参数的函数/类型在后续阶段立即单态化，只是自动实例化的特例。

### 2.2 函数统一处理

全局函数、结构体/共用体方法、局部函数、表达式内函数语义一致，区别仅在符号可见性。定义时注册到当前作用域即可，不按种类做分支。

| 函数类型 | 可见性作用域 | 装饰器 |
|---------|------------|--------|
| 全局函数 | 模块作用域 | 有 |
| 结构体/共用体方法 | 类型作用域（instance/static） | 有 |
| 局部函数 | 函数作用域 | 有 |
| 表达式内函数 | 表达式作用域 | 无 |

### 2.3 类型统一处理

struct/union/enum/interface/type alias 等类型声明语义一致，区别仅在符号可见性。定义时注册到当前作用域，不按种类做分支。

### 2.4 逐作用域递归处理

名字收集 → 定义解析 → 实例化 → 函数体解析在每个作用域内都执行一遍。局部作用域内也可以有函数、类型等声明。

## 3. 静态作用域 vs 非静态作用域

| 作用域类型 | 例子 | 变量 | 函数 | 类型 |
|-----------|------|------|------|------|
| 静态作用域 | 模块（SCOPE_MODULE）、类型（SCOPE_TYPE） | 收集（提升） | 收集（提升） | 收集（提升） |
| 非静态作用域 | 函数体（SCOPE_FUNCTION）、块（SCOPE_BLOCK）、for/foreach | **不收集** | 收集（提升） | 收集（提升） |

- 静态作用域：所有声明名字提升，定义前即可引用
- 非静态作用域：变量不提升（按文本顺序可见），函数和类型仍提升

## 4. 收集内容

遍历当前作用域的一层声明节点，将每个声明的名字注册到当前作用域的 name 表：

| 声明类型 | 注册名字种类 | 备注 |
|---------|------------|------|
| `var` / `const` | NAME_VARIABLE | 仅静态作用域收集 |
| `func` | NAME_FUNCTION | 含方法、局部函数、表达式函数 |
| `struct` / `union` / `enum` / `interface` / `type` | NAME_TYPE | |
| `import` | NAME_NAMESPACE | 导入的名字以 namespace 形式注册，通过 `::` 域作用符访问 |
| `export` | — | 标记当前名字为导出 |

## 5. 模块依赖处理

### 5.1 模块状态

```c
enum module_state {
    MODULE_NEW,        // 未处理
    MODULE_COLLECTING, // 名字收集中
    MODULE_COLLECTED,  // 名字收集完成
    MODULE_RESOLVED,   // 定义解析完成
    MODULE_CHECKED,    // 类型检查完成
};
```

### 5.2 import 处理流程

模块 A 遇到 `import B` 时：

1. 暂停 A 的名字收集
2. 检查 B 的状态：
   - `MODULE_NEW`：执行 B 的名字收集，完成后继续 A
   - `MODULE_COLLECTING`：循环依赖，使用 B 当前已收集到的名字
   - `MODULE_COLLECTED` 或更后：直接使用 B 的导出名字
3. 将 B 的导出名字注册到 A 的作用域
4. 继续 A 的名字收集

### 5.3 export 处理

遇到 `export` 语句时，将对应名字标记为导出。模块名字收集完成后，其他模块可通过 import 引用这些导出名字。

## 6. 递归进入静态子作用域

模块作用域下遇到类型声明（struct/union/interface）时：

1. 先将该类型名字注册到模块作用域（NAME_TYPE）
2. 递归进入类型作用域，收集类型内部的声明（方法、嵌套类型等）

## 7. 与现有代码的集成

### 7.1 现有数据结构

- `scope_t`：已有 `names`（strmap_t）、`children`（vec_t）、`parent`、`kind`、`owner`
- `name_t`：已有 `kind`（NAME_VARIABLE/TYPE/FUNCTION/NAMESPACE）、`ref`（void*）
- `module_t`：已有 `root_scope`、`filename`、`source`、`tokens`、`program`

### 7.2 需新增字段

```c
// module_t 新增
struct _module_t {
    // ... 现有字段 ...
    enum module_state state;  // 模块处理状态
};

// scope_kind 新增
enum scope_kind {
    SCOPE_GLOBAL,
    SCOPE_MODULE,
    SCOPE_FUNCTION,
    SCOPE_BLOCK,
    SCOPE_FOR,
    SCOPE_FOREACH,
    SCOPE_TYPE,      // 新增：类型内部作用域
};
```

### 7.3 API 设计

```c
// engine/name_collector.h

/**
 * @brief 对模块执行名字收集。
 * 遍历模块 program 的一层声明，注册名字到模块 root_scope。
 * 遇到 import 时递归处理依赖模块。
 */
void name_collector_run(context_t ctx, module_t mod);

/**
 * @brief 对静态子作用域执行名字收集。
 * 遍历类型声明的一层成员，注册名字到类型作用域。
 */
void name_collector_run_scope(context_t ctx, scope_t scope, node_t decl);
```

## 8. 管线定位

编译管线按作用域递归执行：

```
对每个作用域：
  1. 名字收集 — 注册当前作用域的声明名字（函数、类型、变量等）
  2. 定义解析 — 解析声明的类型签名、约束等
  3. 实例化   — 无泛型参数的立即单态化，有泛型参数的按需单态化
  4. 函数体解析 — 检查函数体内部，递归进入子作用域
```

入口为模块作用域，遇到 import/export 递归处理依赖模块。
