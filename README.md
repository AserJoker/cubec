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
  - [指针声明](#指针声明)
  - [const 类型表达式](#const-类型表达式)
  - [volatile 类型表达式](#volatile-类型表达式)
  - [切片声明](#切片声明)
  - [数组声明](#数组声明)
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
  - [typeof 表达式](#typeof-表达式)
  - [初始化列表](#初始化列表)
  - [分组类型表达式](#分组类型表达式)
- [类型级三元条件表达式](#类型级三元条件表达式)
- [类型约束表达式](#类型约束表达式)
- [语句](#语句)
- [声明](#声明)
- [模块系统](#模块系统)
- [泛型机制](#泛型机制)
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

**关键字（31个）**：

| | | | | |
|---|---|---|---|---|
| `break` | `case` | `comptime` | `const` | `continue` |
| `defer` | `do` | `else` | `enum` | `export` |
| `extern` | `for` | `foreach` | `func` | `if` |
| `import` | `in` | `inline` | `mutable` | `of` |
| `pub` | `register` | `return` | `struct` | `switch` |
| `test` | `typeof` | `union` | `var` | `volatile` | `while` |

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

### 指针声明

Cubec 支持前缀指针声明语法（区别于 C 的前缀 `*`）：

```c
*i32              // 指向 i32 的指针
* const i32       // const 指针
* volatile i32    // volatile 指针
* const volatile i32  // const volatile 指针
** i32            // 指向指针的指针
* Vec[i32]        // 指向泛型类型的指针
* std::vec::Vec     // 指向命名空间类型的指针
```

在泛型参数中可以使用指针类型：

```c
Vec[* i32]        // Vec 的元素类型是指向 i32 的指针
```

### const 类型表达式

`const` 可作为独立的前缀类型修饰符，与指针、切片地位相当：

```c
const i32              // const 修饰的 i32 类型
const * i32            // const 修饰的指针类型（指向 i32 的 const 指针）
const [] i32           // const 修饰的切片类型
const [10] i32         // const 修饰的数组类型
const Vec[i32]         // const 修饰的泛型类型
const std::vec::Vec      // const 修饰的命名空间类型
const const i32        // 嵌套 const
```

> **注意**：`const * i32` 与 `* const i32` 语义不同。`const * i32` 是 const 修饰整个指针类型，而 `* const i32` 是指针声明中 const 限定符修饰指针本身。

const 类型表达式的 base_type 不直接消费三元表达式，需要通过分组类型表达式包裹：

```c
const (a ? b : c)      // const 修饰的三元类型（需分组）
```

### volatile 类型表达式

`volatile` 与 `const` 地位相当，同样可作为独立的前缀类型修饰符：

```c
volatile i32            // volatile 修饰的 i32 类型
volatile * i32          // volatile 修饰的指针类型
volatile [] i32         // volatile 修饰的切片类型
volatile [10] i32       // volatile 修饰的数组类型
volatile Vec[i32]       // volatile 修饰的泛型类型
volatile std::vec::Vec    // volatile 修饰的命名空间类型
volatile volatile i32   // 嵌套 volatile
```

`volatile` 与 `const` 可自由组合，顺序决定嵌套关系：

```c
const volatile i32      // const 修饰 volatile i32（const 在外层）
volatile const i32      // volatile 修饰 const i32（volatile 在外层）
```

与 const 相同，volatile 的 base_type 不直接消费三元表达式：

```c
volatile (a ? b : c)   // volatile 修饰的三元类型（需分组）
```

### 切片声明

Cubec 支持前缀切片声明语法：

```c
[] i32              // 指向 i32 的切片
[] const i32        // const 切片
[] volatile i32     // volatile 切片
[] const volatile i32  // const volatile 切片
[] Vec[i32]         // 指向泛型类型的切片
[] std::vec::Vec      // 指向命名空间类型的切片
```

> **注意**：`[]` 之间不允许有空白、注释或换行。

在泛型参数中可以使用切片类型：

```c
Vec[[] i32]        // Vec 的元素类型是 [] i32 切片
```

### 数组声明

Cubec 支持定长数组声明语法：

```c
[42] i32              // 42 个 i32 元素的数组
[16] [8] i32          // 二维数组（16 行 8 列）
[N] T                 // 泛型数组（编译期确定大小）
```

数组大小必须是编译期可确定的常量表达式。在泛型参数中可以使用数组类型：

```c
Vec[[10] i32]         // Vec 的元素类型是 [10] i32 数组
```

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

Cubec 使用**后缀语法**表示指针操作和 try 操作（区别于 C 的前缀 `*`/`&`）：

```c
value.*     // 解引用（读取指针指向的值）
value.&     // 取地址（获取 value 的指针）
value.?     // Try/unwrap（安全地解包 Result/Option 类型）
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

`...` 在词法层面被识别为独立的三字符运算符，与 `&&=`、`\|\|=` 同级匹配。

---

## 表达式

### 分组表达式

```c
(a + b) * c
```

括号用于改变求值顺序。

### 函数调用

```c
callee(arg1, arg2, ...)
```

- 支持零参数：`foo()`
- 支持展开参数：`fn(...args)`
- 支持链式调用：`foo()()`
- 支持混合命名空间/类型成员访问：`obj.method()`, `foo().field`, `Vec::create()`

### 成员访问

```c
host.field
host.nested.field       // 链式实例成员访问
```

- `.` 仅用于对象/变量的**实例成员**访问（字段和方法）

### 命名空间/类型成员访问

```c
std::vec::Vec           // 命名空间导航
File::open("a.txt","r") // 类型静态方法调用
Vec::new()              // 类型静态方法调用
```

- `::` 仅用于**类型级**访问：命名空间导航和类型静态成员/方法
- 类型表达式中使用 `::`：`*std::vec::Vec` → 指向 `std::vec::Vec` 的指针
- 混合使用：`std::Vec::new().field` → `::` 访问静态方法，`.` 访问实例字段

### 泛型实例化

使用方括号语法表示泛型参数：

```c
Vec[i32]                // 单参数
Map[string, i32]        // 多参数
fn[T](arg)              // 函数泛型调用
fn[a][b]                // 链式实例化
```

> 与切片表达式的歧义通过 `:` 消解：`arr[0:10]` 是切片，`arr[0]` 是泛型实例化。单参数形式的最终语义留待语义分析阶段确定。

### 切片表达式

```c
host[start:length]      // 完整切片
host[start:]            // 省略 length（截取到末尾）
host[:length]           // 省略 start（从开头截取）
arr[0:1][1:2]           // 链式切片
```

### typeof 表达式

`typeof` 是编译期类型计算表达式，用于在类型注解中获取一个表达式的类型，而不实际执行该表达式：

```c
typeof(x)              // 获取变量 x 的类型
typeof(a + b)          // 获取表达式 a + b 的结果类型
typeof(foo())          // 获取函数返回类型
```

`typeof` 在语法上被视为**顶层类型表达式**（与三元类型同级），可以用于任何需要类型的地方。支持后缀命名空间访问和泛型实例化，但不能被指针/切片声明包裹：

```c
typeof(File)::open     // typeof 结果 + 命名空间访问（类型静态方法）
typeof(Vec)[i32]       // typeof 结果的泛型实例化
```

> **注意**：`typeof` 仅在编译期计算类型，内部表达式不会在运行时求值。`typeof` 是顶层类型表达式，与三元类型平级，不能作为指针/切片的 base_type。

### 初始化列表

Cubec 使用 `.<type>{<items>}` 语法创建初始化列表（结构体/容器字面量），类型可选：

```c
.Vec{1, 2, 3}              // 带类型的初始化列表
.{1, 2, 3}                 // 匿名初始化列表
.Vec{}                      // 空初始化列表
.{.x = 1, .y = 2}          // 指定字段名
```

**items 的两种模式**（不可混用）：

1. **字段模式**：每个 item 为 `.field = value`
2. **位置模式**：每个 item 为普通表达式

**字段与表达式的区分**：以 `=` 为关键区分符：

```c
.{.Test = 123}             // 字段模式：.Test 是字段名，123 是值
.{.Test{}}                 // 位置模式：.Test{} 是一个嵌套的初始化列表表达式
```

类型支持复杂类型表达式（命名空间访问、泛型实例化、指针等）：

```c
.std::vec::Vec{1, 2, 3}    // 命名空间访问类型
.Vec[i32]{1, 2, 3}         // 泛型实例化类型
.* i32{1, 2}               // 指针类型
```

初始化列表可参与后缀链和二元表达式：

```c
.Vec{1, 2}.field           // 初始化列表 + 成员访问
1 + .Vec{1, 2}             // 初始化列表在二元表达式中
```

> **错误**：trailing comma（`.{1, }`）、unclosed brace（`.{1`）、混用字段与位置模式（`.{1, .x = 2}`）均会触发解析错误。

### 分组类型表达式

用于在类型注解中明确分组复杂类型：

```c
([]i32)              // 分组切片类型
(*i32)               // 分组指针类型
(Vec[i32])           // 分组泛型类型
(Ptr[T])             // 分组泛型指针类型
```

分组类型表达式使用括号将类型表达式包装，用于澄清复杂类型注解的优先级，或者在需要明确分组的地方使用。例如 `Vec[[]i32]` 中的内层 `[]i32` 需要分组来表示切片类型作为泛型参数。

### 类型级三元条件表达式

用于类型注解中的编译期类型分支选择。condition 支持三种形式：

- **类型约束**：`T extends U`、`T == U`、`T != U`（见下节）
- **编译期表达式**：括号包裹的值级表达式，如 `(1)`、`(a + b)`
- **简单类型表达式**：标识符、指针、切片等

```c
a ? Vec[i32] : f32              // 简单类型三元
( a ) ? b : c                   // 括号分组 condition
( a ? b : c ) ? d : e           // 嵌套三元（需括号分组，避免无限递归）
a ? Vec[i32] : f32              // consequent 支持泛型实例化
T extends U ? X : Y             // 类型约束作为 condition
T == i32 ? Vec[T] : T           // 类型等值约束作为 condition
( a + b ) ? X : Y               // 编译期表达式作为 condition
( 1 ) ? a : b                   // 编译期字面量条件
```

类型级三元与值级三元共用 `? :` 语法，但语义不同：类型级三元在编译期类型推导时求值，用于泛型约束中的类型分支选择。指针/切片/数组的 base_type 使用基础类型表达式解析（不含三元），因此三元表达式不能直接作为 base_type。如果需要将三元类型作为 base_type，必须通过分组类型表达式包裹：

```c
* a ? b : c      // → (*a) ? b : c   指针作为三元条件
[] a ? b : c     // → ([]a) ? b : c   切片作为三元条件
[10] a ? b : c   // → ([10]a) ? b : c 数组作为三元条件

* (a ? b : c)    // → *(a ? b : c)   指针指向三元类型（需分组）
[] (a ? b : c)   // → [](a ? b : c)  切片指向三元类型（需分组）
[10] (a ? b : c) // → [10](a ? b : c) 数组指向三元类型（需分组）
```

条件表达式 `a ? : b`（缺少 consequent）和 `a ? b :`（缺少 alternate）会触发解析错误。

### 类型约束表达式

类型约束是类型级三元的一种 condition 形式，用于编译期类型关系判断。支持三种运算符：

```c
T extends U     // 子类型检查：T 是否是 U 的子类型
T == U          // 类型相等检查
T != U          // 类型不等检查
```

作为类型级三元的 condition 使用：

```c
T extends Vec[i32] ? T : i32      // extends 约束
T == i32 ? Vec[ T ] : T           // == 约束
T != f64 ? f32 : T                // != 约束
* (T extends U ? X : Y)          // 指针指向约束三元（需分组）
```

`extends` 是关键词，`==` 和 `!=` 是运算符。右操作数仅接受基础类型表达式，确保 `T extends U ? X : Y` 被正确解析为 `(T extends U) ? X : Y`。

除了类型约束，condition 还可以是括号包裹的编译期表达式（如 `(a + b) ? X : Y`）或简单类型标识符（见 [类型级三元条件表达式](#类型级三元条件表达式)）。

> **注意**：独立的类型约束表达式（如 `T extends U` 不带 `?`）在类型表达式中不合法——它们只在类型三元条件或泛型定义上下文中有效。

---

## 语句

> **TODO**: 以下语句语法设计已完成，部分解析器待实现。

### 变量声明

```c
var x = 42;                    // 自动类型推导
var x: i32 = 42;               // 显式类型注解
var a = 1, b = 2, c = 3;       // 多变量声明
var vec = .Vec{1, 2, 3};       // 初始化列表作为值
var point = .Point{.x = 1, .y = 2};  // 带字段的初始化列表
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

### 块语句

```c
{                   // 块语句：引入新作用域
    let x: i32 = 1;
    foo();
}

{}                  // 空块语句
{ { ; } }           // 嵌套块
```

块语句由一对花括号包裹的语句序列组成，用于引入新的作用域。块可以为空。

### 表达式语句

```c
some_function();       // 函数调用语句
1 + 2;                 // 表达式求值语句（结果被丢弃）
std::Vec::create();    // 命名空间静态方法调用语句
```

表达式语句由一个表达式后跟分号 `;` 组成。缺少分号是语法错误。

---

## 声明

> **TODO**: 以下声明语法设计已完成，解析器待实现。

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

## 泛型机制

Cubec 的泛型基于**"推导 + 鸭子类型"**范式，采用编译期模板实现。无函数重载，`extends` 校验基于结构兼容性，大幅降低复杂度。

### 设计原则

| 原则 | 说明 |
|------|------|
| 推导优先 | 从实参类型推导泛型参数，失败则编译报错（支持显式指定 `parse[i32]("42")`） |
| 鸭子类型 | `T extends IWriter` 判定 T 是否具备 IWriter 的操作，非继承链 |
| 无重载 | 每函数名唯一实现，无 SFINAE |
| `[]` 语法 | 泛型统一使用方括号，避免与 `<>` 比较运算符歧义 |
| 编译期模板 | 实例化在编译期完成，无运行时开销 |

### 各类型泛型支持

| 类型 | 泛型 | 推导 | 语义 |
|------|------|------|------|
| `struct` | ✅ | ❌ 无构造函数，显式 `Vec[i32]{}` | 模板实例化 |
| `enum` | ❌ | — | 所有成员同类型（TS 风格） |
| `union` | ✅ | — | tagged union（Rust 风格） |
| `interface` | ✅ | — | 仅方法签名（Go 风格结构型） |
| `func` | ✅ | ✅ | 自动推导 + 显式标注双模 |

### 语法速览

```c
// === 泛型参数定义 ===
func[T](x: T) -> T                           // 推导
func[T extends Numeric](x: T) -> T           // 带约束
func[N: u64, T](arr: [N]T) -> T             // 值泛型 + 类型泛型
func[T, ...Rest](first: T, rest: Rest) -> T  // rest 参数

// === 类型级运算 ===
func[T](x: T) -> T extends Numeric ? i64 : string   // 类型三元
func[T](x: T) -> T == i32 ? f64 : string            // 类型相等
func[T](x: T) -> T != void ? T : i32                // 类型不等

// === 嵌套解包推导 ===
func[T](x: Vec[T])       // Vec[i32] → T = i32
func[K, V](x: Map[K, V]) // Map[string, i32] → K = string, V = i32

// === 通配符 ?（仅 extends 约束中） ===
func[T extends Array[?]](arr: T)             // 任意元素类型的 Array

// === type 别名 ===
type Vec3[T] = Vec[Vec[Vec[T]]]
type Pair[A, B] = struct { first: A, second: B }
type Variadic[...Args] = i32                 // rest 参数别名

// === comptime if ===
func[T](x: T) {
    comptime if (T extends Numeric) {
        print("numeric: ", x)
    } else {
        print("other")
    }
}

// === 递归泛型 ===
struct List[T] { head: T; tail: List[T] }

// === struct 关联类型 + 独立泛型方法 ===
struct Vec[T] {
    data: *T; len: u64
    type Element = T
    func[U](self: Vec[T], other: Vec[U]) -> Vec[T] { ... }
}

// === interface 泛型 + 关联类型 ===
interface Iterator {
    type Item
    next(self): Item
}

interface Container[T] {
    len(self): u64
    get(self, idx: u64): T
}

interface Mapper[K, V] {
    map(self, key: K): V
}
```

### builtin 编译器指令

语言级类型变换使用 `builtin` 指令声明：

```c
builtin RemoveConst[T extends const?]       // → 剥离 const
builtin RemoveVolatile[T extends volatile?] // → 剥离 volatile
builtin Pointer[T]                          // → *T
builtin Slice[T]                            // → []T
builtin RemovePointer[T extends *?]         // → 解指针
builtin RemoveSlice[T extends []?]          // → 解切片
builtin ReturnType[F extends func]          // → 返回类型
builtin SizeOf[T]                           // → u64（编译期值）
```

直接作为类型表达式使用：

```c
type Mutable[T] = RemoveConst[T]
type Ptr[T] = Pointer[T]
type SlicePtr[T] = Slice[Pointer[T]]
```

> **注意**：容器元素类型（如 `Vec[i32].Element`）由容器自身通过 struct 内 `type` 定义暴露，非编译器内置。

### 类型谓词体系

```
判定层：extends（鸭子约束）、==（双向等价 → 结构等价）、!=（取反）
变换层：builtin 编译器指令
分支层：类型级三元（? :）、comptime if
推导层：嵌套解包、多位置统一、全类型编译期计算
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
├── test/             # 测试文件（Google Test, 600 测试用例）
├── demo/             # 示例 .cubec 文件
└── third_party/      # 第三方依赖
```

---

## 实现进度

### 已实现 ✅

- **词法分析器**：完整的 tokenizer，支持所有字面量类型、30 个关键字、符号最长匹配
- **表达式解析器**：
  - 前缀一元表达式（`!`, `+`, `-`, `~`）
  - 后缀一元表达式（`.*` 解引用, `&` 取地址, `.?` try/unwrap）
  - 二元表达式（10 级优先级）
  - 赋值表达式（`=`, `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`, `^=`, `<<=`, `>>=`, `&&=`, `||=`）
  - 三元条件表达式（`? :`）
  - 分组表达式（`(expr)`）
  - 函数调用表达式（`callee(args)`）
  - 成员访问表达式（`host.field`，实例成员）
  - 命名空间/类型成员访问表达式（`host::field`，命名空间导航 + 静态成员/方法）
  - 泛型实例化表达式（`callee[args]`）
  - 切片表达式（`host[start:length]`）
  - 展开表达式（`...expr`）
  - 初始化列表表达式（`.<type>{<items>}` 或 `.{<items>}`，支持字段/位置模式，类型支持复杂类型表达式）
  - 初始化字段表达式（`.field = value`，初始化列表的字段项）
  - typeof 表达式（`typeof(<expression>)`，编译期类型计算，顶层类型表达式，支持后缀 `::`/`[]`，不能作为指针/切片的 base_type）
  - 类型表达式（用于类型标注，含指针声明 `* [const] [volatile] <type>`）
  - 分组类型表达式（`( type_expression )`）
  - 类型级三元条件表达式（`condition ? type : type`，编译期类型分支，condition 支持类型约束/编译期表达式/简单标识符）
  - 类型约束表达式（`T extends U`, `T == U`, `T != U`，作为类型三元条件使用，bare 形式报错）
  - const 类型表达式（`const <type>`，独立前缀类型修饰符，与指针/切片地位相当）
  - volatile 类型表达式（`volatile <type>`，独立前缀类型修饰符，与 const/指针/切片地位相当）
  - 数组声明表达式（`[ <expr> ] <type>`）
- **语句解析器**：
  - 块语句（`{ <statements> }`，引入新作用域，支持空块和嵌套）
  - 表达式语句（`<expression>;`，缺少分号为语法错误）
  - 空语句（`;`）
  - 变量声明语句（`var <identifier> [: <type>] = <expression> [, ...];`，支持多变量逗号分隔）
  - `type` 别名声明语句（`type Name[<generic_params>] = <type_expression>;`，支持简单别名、泛型别名、rest 参数）
  - 泛型参数解析（支持简单 `T`、约束 `T extends U`、值泛型 `N: u64`、rest 参数 `...Args` 四种形式）
  - `read_statement` 语句分派器（按优先级尝试各语句类型）
- **核心数据结构**：动态数组、双向链表、红黑树、哈希表、动态字符串、统一内存管理
- **测试体系**：627 测试用例覆盖所有核心模块

### 待实现 📋

- **语句解析**（AST 节点已定义）：
  - `if`/`else` 条件语句
  - `for`/`foreach`/`while`/`do-while` 循环语句
  - `switch`/`case` 分支语句
  - `defer` 延迟执行
  - `break`/`continue`/`return` 跳转语句
- **声明解析**：
  - `func` 函数声明（含泛型）
  - `struct` 结构体声明
  - `enum` 枚举声明
  - `union` 联合体声明
  - 变量声明（`var`，已实现 basic 形式）
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
