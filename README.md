# Cubec Programming Language

**Cubec** 是一门类 C 的静态类型编程语言，设计目标是提供现代语法特性（泛型、defer、comptime、模式匹配等），同时保持简洁和可预测的语义。

> **状态**：前端编译器开发中。词法分析器、语法分析器和语义分析引擎已实现，comptime 编译期求值器已完成。

---

## 快速示例

```c
import std.io;

func main(): void {
    var greeting = "Hello, Cubec!";
    io.println(greeting);

    var numbers = .Vec[i32]{1, 2, 3, 4, 5};
    foreach(var n of numbers) {
        io.println(n);
    }
}
```

---

## 文档

详细设计文档位于 [`docs/`](docs/) 目录：

| 文件 | 内容 |
|------|------|
| [`docs/01-architecture.md`](docs/01-architecture.md) | 核心架构：编译流程、TDZ、作用域与符号表 |
| [`docs/02-type-system.md`](docs/02-type-system.md) | 类型系统：双层架构、结构等价、const/volatile、魔法方法 |
| [`docs/03-pointer-semantics.md`](docs/03-pointer-semantics.md) | 指针语义：退化规则、const 指针、spread 与退化 |
| [`docs/04-expressions.md`](docs/04-expressions.md) | 表达式：成员调用 desugaring、闭包、lvalue/const、展开运算符 |
| [`docs/05-control-flow.md`](docs/05-control-flow.md) | 控制流：if/for/foreach/switch/defer |
| [`docs/06-modules.md`](docs/06-modules.md) | 模块系统：import/export、路径解析、循环依赖 |
| [`docs/07-comptime.md`](docs/07-comptime.md) | Comptime 引擎：虚拟指针、值表示、test/build |
| [`docs/08-generics.md`](docs/08-generics.md) | 泛型：单态化、extends 约束、参数包 |
| [`docs/09-modifiers.md`](docs/09-modifiers.md) | 修饰符：builtin/extern/register/comptime/inline/export/pub |
| [`docs/10-union-interface.md`](docs/10-union-interface.md) | Union（tagged + cunion）与 Interface 语义 |
| [`docs/11-codegen.md`](docs/11-codegen.md) | 代码生成：C 后端映射 |
| [`docs/12-syntax-reference.md`](docs/12-syntax-reference.md) | 语法参考：关键字、类型、运算符、表达式、语句、声明 |
| [`docs/13-ast-semantics.md`](docs/13-ast-semantics.md) | AST 节点语义与 checker 模块结构 |

---

## 构建与开发

### 环境要求

| 项目 | 版本 |
|------|------|
| CMake | ≥ 3.12 |
| C 标准 | C11 (GNU extensions) |
| C++ 标准 | C++20 |
| ICU | 74.2 |
| Google Test | 1.15.2 |

### 构建

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

### 运行测试

```bash
ctest
# 或直接运行
./cubec_test
```

### 项目结构

```
cubec/
├── include/          # 头文件
│   ├── core/         # 核心数据结构（vec, list, rbtree, map, string 等）
│   └── cubec/        # 前端模块（词法/语法分析）
├── src/              # 源文件（与 include/ 对应）
├── test/             # 测试文件（Google Test）
├── demo/             # 示例 .cubec 文件
└── third_party/      # 第三方依赖
```

---

## 实现进度

### 已实现

- **词法分析器**：完整的 tokenizer，支持所有字面量类型、40 个关键字、符号最长匹配
- **表达式解析器**：完整覆盖所有表达式类型（前缀/后缀一元、二元、三元、成员访问、泛型实例化、typeof、sizeof、alignof、初始化列表、匿名函数、const/volatile 限定符等）
- **语句解析器**：完整覆盖所有语句类型（if/for/foreach/while/do-while/switch/defer/break/continue/return/import/test/comptime 等）
- **声明解析器**：func/struct/enum/union/cunion/interface/type/var/decorator
- **语义分析引擎**：双层类型表示、结构等价、指针退化、TDZ 多遍检查、const/volatile 语义、builtin 动态注册表、参数包（variadic generics）、泛型推断、Rustc 风格诊断、`undefined` 字面量与强制变量初始化
- **comptime 编译期求值器**：AST 解释器、虚拟内存、安全限制、test 块执行
- **核心数据结构**：动态数组、双向链表、红黑树、哈希表、动态字符串、统一内存管理

### 待实现

- **代码生成**（后端）
