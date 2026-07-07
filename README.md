# Cubec Programming Language

**Cubec** 是一门类 C 的静态类型编程语言，设计目标是提供现代语法特性（泛型、defer、comptime、模式匹配等），同时保持简洁和可预测的语义。

> **状态**：前端编译器开发中。词法分析器已完整实现，表达式解析器基本完成，语句和声明部分待实现。

---

## 目录

- [快速示例](#快速示例)
- [基础语法](#基础语法)
  - [注释](#注释)
  - [字面量](#字面量)
  - [标识符与关键字](#标识符与关键字)
- [类型系统](#类型系统)
  - [基本类型](#基本类型)
  - [类型后缀](#类型后缀)
- [运算符](#运算符)
  - [前缀一元运算符](#前缀一元运算符)
  - [后缀一元运算符](#后缀一元运算符)
  - [二元运算符与优先级](#二元运算符与优先级)
  - [三元条件运算符](#三元条件运算符)
  - [展开运算符](#展开运算符)
- [表达式](#表达式)
  - [分组表达式](#分组表达式)
  - [函数调用](#函数调用)
  - [成员访问](#成员访问)
  - [泛型实例化](#泛型实例化)
  - [切片表达式](#切片表达式)
- [语句](#语句)
- [声明](#声明)
- [模块系统](#模块系统)
- [特殊特性](#特殊特性)
- [构建与开发](#构建与开发)
- [实现进度](#实现进度)

---

## 快速示例

```c
import std.io;

func main() {
    let greeting = "Hello, Cubec!";
    io.println(greeting);

    let numbers = Vec[i32]{1, 2, 3, 4, 5};
    for n in numbers {
        io.println(n);
    }
}
```

---

## 基础语法

### 注释

```c
// 单行注释

/*
 * 多行注释
 * 不支持嵌套
 */
```

### 字面量

**数值字面量** 支持多种进制和科学计数法：

| 进制 | 前缀 | 示例 |
|------|------|------|
| 十进制 | 无 | `42`, `3.14` |
| 十六进制 | `0x` | `0xFF`, `0x1A` |
| 八进制 | `0o` | `0o755` |
| 二进制 | `0b` | `0b1010` |
| 科学计数法 | `e`/`E` | `1.5e10`, `2.0E-3` |

**字符字面量** 用单引号，支持转义：

```c
'a', '\n', '\t', '\\', '\'', '\"', '\0', '\x41', '\u{1F600}'
```

**字符串字面量** 用双引号，支持相同转义序列。相邻字符串自动拼接：

```c
"hello world"
"line1\n"
"line2\n"
// 等价于: "line1\nline2\n"
```

### 标识符与关键字

标识符遵循 Unicode 标准（通过 ICU `u_isIDStart`/`u_isIDPart` 识别），支持非 ASCII 字符。

**关键字（29个）**：

| | | | | |
|---|---|---|---|---|
| `break` | `case` | `comptime` | `const` | `continue` |
| `defer` | `do` | `else` | `enum` | `export` |
| `extern` | `for` | `foreach` | `func` | `if` |
| `import` | `in` | `inline` | `mutable` | `of` |
| `pub` | `register` | `return` | `struct` | `switch` |
| `test` | `union` | `volatile` | `while` | |

---

## 类型系统

### 基本类型

Cubec 提供标准整数和浮点类型：

| 类型 | 描述 |
|------|------|
| `i8`, `i16`, `i32`, `i64` | 有符号整数 |
| `u8`, `u16`, `u32`, `u64` | 无符号整数 |
| `f16`, `f32`, `f64` | 浮点数 |

> **TODO**: `f16` 语义待确认（是否使用 IEEE 754 binary16，或映射到 `f32`）。

### 类型后缀

数值字面量可附带类型后缀指定精度：

```c
42i32       // 有符号 32 位整数
255u8       // 无符号 8 位整数
3.14f64     // 64 位浮点数
```

---

## 运算符

### 前缀一元运算符

```c
!value      // 逻辑非
-value      // 算术取负
+value      // 一元正号
~value      // 按位取反
```

支持链式：`!!x`, `--n`（双重取负即为正）。

### 后缀一元运算符

Cubec 使用**后缀语法**表示指针操作（区别于 C 的前缀 `*`/`&`）：

```c
value.*     // 解引用（读取指针指向的值）
value.&     // 取地址（获取 value 的指针）
```

### 二元运算符与优先级

运算符按优先级从低到高：

| 优先级 | 运算符 | 结合性 |
|--------|--------|--------|
| 1 | `\|\|` | 左结合 |
| 2 | `&&` | 左结合 |
| 3 | `\|` | 左结合 |
| 4 | `^` | 左结合 |
| 5 | `&` | 左结合 |
| 6 | `==` `!=` | 左结合 |
| 7 | `<` `>` `<=` `>=` | 左结合 |
| 8 | `<<` `>>` | 左结合 |
| 9 | `+` `-` | 左结合 |
| 10 | `*` `/` `%` | 左结合 |

> **注意**：赋值运算符（`=`、`+=` 等）和逗号运算符（`,`）目前**不作为**二元表达式处理，将在语句层面单独实现。

### 三元条件运算符

```c
condition ? consequent : alternate
```

支持嵌套：`a ? b ? c : d : e`，解析规则为右结合。

### 展开运算符

`...` 运算符用于展开参数或结构体字段：

```c
func(...args)          // 在函数调用中展开参数
Type[...type_args]     // 在泛型参数中展开类型
```

`...` 是词法层面的 `SYMBOL` token，通过三字符最长匹配与 `&&=`、`\|\|=` 同组识别。

---

## 表达式

### 分组表达式

```c
(a + b) * c
```

括号用于改变求值顺序，解析为 `expression_group` 节点。

### 函数调用

```c
callee(arg1, arg2, ...)
```

- 支持零参数：`foo()`
- 支持展开参数：`fn(...args)`
- 支持链式调用：`foo()()`
- 支持混合成员访问：`obj.method()`, `foo().field`

### 成员访问

```c
host.field
host.nested.field       // 链式成员访问
```

### 泛型实例化

使用方括号语法表示泛型参数：

```c
Vec[i32]                // 单参数
Map[string, i32]        // 多参数
fn[T](arg)              // 函数泛型调用
fn[a][b]                // 链式实例化
```

> 与切片表达式的歧义通过冒号前瞻（`:` lookahead）消解：`arr[0:10]` 是切片，`arr[0]` 是泛型实例化。单参数形式的最终语义留待语义分析阶段确定。

### 切片表达式

```c
host[start:length]      // 完整切片
host[start:]            // 省略 length（截取到末尾）
host[:length]           // 省略 start（从开头截取）
arr[0:1][1:2]           // 链式切片
```

---

## 语句

> **TODO**: 以下语句类型已在 AST 节点枚举中定义，但解析器尚未实现。

### 变量声明

```c
// TODO
let x: i32 = 42;
const PI: f64 = 3.14159;
mutable counter: u64 = 0;
```

### 条件语句

```c
// TODO
if condition {
    // ...
} else if other {
    // ...
} else {
    // ...
}
```

### 循环语句

```c
// TODO
for i in 0..10 {
    // 范围循环
}

// foreach 循环
foreach item in collection {
    // ...
}

// while 循环
while condition {
    // ...
}

// do-while 循环
do {
    // ...
} while condition;
```

### 分支语句

```c
// TODO
switch value {
    case 1 => { /* ... */ }
    case 2 | 3 => { /* ... */ }
    else => { /* ... */ }
}
```

### 跳转语句

```c
// TODO
break;
continue;
return value;
```

### defer 语句

```c
// TODO
defer cleanup();    // 在当前作用域退出时执行
```

### 表达式语句

```c
// TODO (解析器支持了表达式，但尚未集成到语句层面)
some_function();
1 + 2;
```

---

## 声明

> **TODO**: 以下声明类型解析器尚未实现。

### 函数声明

```c
// TODO
func add(a: i32, b: i32) -> i32 {
    return a + b;
}

// 泛型函数
func identity[T](value: T) -> T {
    return value;
}
```

### 结构体声明

```c
// TODO
struct Point {
    x: f64,
    y: f64,
}

// 泛型结构体
struct Vec[T] {
    data: *T,
    len: usize,
}
```

### 枚举声明

```c
// TODO
enum Color {
    Red,
    Green,
    Blue,
}

enum Option[T] {
    Some(T),
    None,
}
```

### 联合体声明

```c
// TODO
union Data {
    int_val: i32,
    float_val: f64,
}
```

---

## 模块系统

```c
// TODO
import std.io;              // 导入模块
pub func exported() { }     // 公开导出
export func global() { }    // 另一种导出方式
```

---

## 特殊特性

### comptime 编译时求值

```c
// TODO
comptime {
    // 在编译时执行的代码块
}

const computed: i32 = comptime expensive_calc();
```

### test 块

```c
// TODO
test "feature X works" {
    assert(add(1, 2) == 3);
}
```

### decorator 装饰器

```c
// TODO
@inline
func hot_function() { }

@deprecated
func old_api() { }
```

### extern 外部函数

```c
// TODO
extern "C" {
    func printf(format: *u8, ...) -> i32;
}
```

### inline 内联

```c
// TODO
inline func fast_add(a: i32, b: i32) -> i32 {
    return a + b;
}
```

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
├── test/             # 测试文件（Google Test, 346+ 测试用例）
├── demo/             # 示例 .cubec 文件
└── third_party/      # 第三方依赖
```

---

## 实现进度

### 已实现 ✅

- **词法分析器**：完整的 tokenizer，支持所有字面量类型、29 个关键字、符号最长匹配
- **表达式解析器**（Precedence Climbing）：
  - 前缀一元表达式（`!`, `+`, `-`, `~`）
  - 后缀一元表达式（`.*` 解引用, `&` 取地址）
  - 二元表达式（10 级优先级）
  - 赋值表达式（`=`, `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`, `^=`, `<<=`, `>>=`, `&&=`, `||=`）
  - 三元条件表达式（`? :`）
  - 分组表达式（`(expr)`）
  - 函数调用表达式（`callee(args)`）
  - 成员访问表达式（`host.field`）
  - 泛型实例化表达式（`callee[args]`）
  - 切片表达式（`host[start:length]`）
  - 展开表达式（`...expr`）
  - 类型表达式（用于类型标注）
  - 空语句（`;`）
- **核心数据结构**：vec, list, rbtree, map, string, allocator（含内存泄漏检测）
- **测试体系**：346+ 测试用例覆盖所有核心模块

### 待实现 📋

- **语句解析**（AST 节点已定义）：
  - `if`/`else` 条件语句
  - `for`/`foreach`/`while`/`do-while` 循环语句
  - `switch`/`case` 分支语句
  - `defer` 延迟执行
  - `break`/`continue`/`return` 跳转语句
  - 块语句 `{}`
  - 表达式语句
- **声明解析**：
  - `func` 函数声明（含泛型）
  - `struct` 结构体声明
  - `enum` 枚举声明
  - `union` 联合体声明
  - 变量声明（`let`/`const`/`mutable`）
- **模块系统**：
  - `import` 导入
  - `pub`/`export` 导出
- **高级特性**：
  - `comptime` 编译时求值
  - `test` 测试块
  - `decorator` 装饰器 / 属性
  - `extern` 外部链接
  - `inline` 内联
- **语义分析**（`src/engine/` — 目录尚未创建）
- **代码生成**（后端 — 待定）
