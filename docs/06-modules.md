# Cubec 模块系统

## 1. import 解析

```c
import std from "std";
import vec as v from "std/vec";
```

- 解析路径 → 定位 `.cubec` 文件
- 解析该文件 → 生成 AST
- 执行声明收集 → 构建模块符号表
- 只暴露 `export` 标记的符号
- 模块符号必须通过 `模块名::` 访问

### 1.1 as 重命名

`as` 关键字用于 import 语句的重命名，为模块指定别名：

```c
import vec as v from "std/vec";    // 用 v:: 代替 vec:: 访问
import collections as col from "std/collections";
```

---

## 2. 循环依赖

使用延迟解析 + 模块缓存（类似 Node.js）：

```c
enum module_state {
    MODULE_PARSING,     // 正在声明收集，可能不完整
    MODULE_PARSED,      // 声明收集完成
    MODULE_CHECKED,     // 类型检查完成
};
```

- 遇到已开始解析但未完成的模块 → 返回当前已收集的 export 符号
- PARSING 状态模块的符号允许名字引用（NameKnown），值引用（TDZ）报错
- 与 TDZ 机制完全一致，不需要额外规则

---

## 3. export 模块级导出

`export` 修饰声明使其对其他模块可见：

```c
export func add(a: i32, b: i32): i32 { return a + b; }
export type Point = struct { x: i32; y: i32; }
export var VERSION: const string = "1.0";
```

- 未 `export` 的符号仅在当前模块内可见
- `export` 与 `pub` 职责不同，正交：
  - `export` — 模块级，控制跨模块可见性
  - `pub` — 字段级，控制 struct 字段可见性（详见 `09-modifiers.md`）
- `export` 可与 `builtin`、`comptime`、`inline` 组合使用
