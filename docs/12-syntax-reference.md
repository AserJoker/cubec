# Cubec 语法参考

## 1. 基础语法

### 1.1 注释

```c
// 单行注释

/*
 * 多行注释
 * 不支持嵌套
 */
```

### 1.2 字面量

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

### 1.3 标识符与关键字

标识符遵循 Unicode 标准（通过 ICU `u_isIDStart`/`u_isIDPart` 识别），支持非 ASCII 字符。

**关键字（40个）**：

| | | | | |
|---|---|---|---|---|
| `alignof` | `as` | `break` | `builtin` | `case` |
| `comptime` | `const` | `continue` | `cunion` | `defer` |
| `do` | `else` | `enum` | `export` | `extends` |
| `extern` | `for` | `foreach` | `from` | `func` |
| `if` | `import` | `in` | `inline` | `interface` |
| `is` | `of` | `pub` | `register` | `return` |
| `sizeof` | `struct` | `switch` | `test` | `type` |
| `typeof` | `union` | `var` | `volatile` | `while` |

> `mutable` 已移除。`void` 是预定义标识符，非关键字。

---

## 2. 类型系统

### 2.1 基本类型

| 类型 | 描述 |
|------|------|
| `void` | 空类型（预定义标识符，非关键字） |
| `i8`, `i16`, `i32`, `i64` | 有符号整数 |
| `u8`, `u16`, `u32`, `u64` | 无符号整数 |
| `f16`, `f32`, `f64` | 浮点数（`f16` 为 IEEE 754 binary16 半精度浮点） |

### 2.2 指针声明

Cubec 支持前缀指针声明语法（区别于 C 的前缀 `*`）：

```c
*i32              // 指向 i32 的指针
* const i32       // 指向 const i32 的指针（C: const int*）
* volatile i32    // 指向 volatile i32 的指针（C: volatile int*）
* const volatile i32  // 指向 const volatile i32 的指针
** i32            // 指向指针的指针
* Vec[i32]        // 指向泛型类型的指针
* std::vec::Vec   // 指向命名空间类型的指针
```

在泛型参数中可以使用指针类型：

```c
Vec[* i32]        // Vec 的元素类型是指向 i32 的指针
```

### 2.3 const 类型表达式

`const` 可作为独立的前缀类型修饰符，与指针、切片地位相当：

```c
const i32              // const 修饰的 i32 类型
const * i32            // const 指针（C: int* const），指针本身不可重赋值
const [] i32           // const 修饰的切片类型
const [10] i32         // const 修饰的数组类型
const Vec[i32]         // const 修饰的泛型类型
const std::vec::Vec    // const 修饰的命名空间类型
const volatile i32     // const + volatile 修饰
```

> **注意**：`const * i32` 与 `* const i32` 语义不同。`const * i32` 是 const 修饰指针本身（C: `int* const`），指针不可重赋值但可通过指针修改数据；`* const i32` 是指针指向 const 数据（C: `const int*`），数据不可通过指针修改但指针可重赋值。

const 类型表达式贪婪消费内部类型，包括三元：

```c
const a ? b : c    // → const(ternary(a, b, c))
const (a) ? b : c  // → ternary(const(a), b, c)（需分组消歧）
```

### 2.4 volatile 类型表达式

`volatile` 与 `const` 地位相当，同样可作为独立的前缀类型修饰符：

```c
volatile i32            // volatile 修饰的 i32 类型
volatile * i32          // volatile 修饰的指针类型
volatile [] i32         // volatile 修饰的切片类型
volatile [10] i32       // volatile 修饰的数组类型
volatile Vec[i32]       // volatile 修饰的泛型类型
volatile std::vec::Vec  // volatile 修饰的命名空间类型
volatile volatile i32   // 嵌套 volatile
```

`volatile` 与 `const` 可自由组合，顺序决定嵌套关系：

```c
const volatile i32      // const 修饰 volatile i32（const 在外层）
volatile const i32      // volatile 修饰 const i32（volatile 在外层）
```

与 const 相同，volatile 贪婪消费内部类型，包括三元：

```c
volatile a ? b : c    // → volatile(ternary(a, b, c))
volatile (a) ? b : c  // → ternary(volatile(a), b, c)（需分组消歧）
```

### 2.5 切片声明

```c
[] i32              // 指向 i32 的切片
[] const i32        // const 切片
[] volatile i32     // volatile 切片
[] const volatile i32  // const volatile 切片
[] Vec[i32]         // 指向泛型类型的切片
[] std::vec::Vec    // 指向命名空间类型的切片
```

> **注意**：`[]` 之间不允许有空白、注释或换行。

在泛型参数中可以使用切片类型：

```c
Vec[[] i32]        // Vec 的元素类型是 [] i32 切片
```

### 2.6 数组声明

```c
[42] i32              // 42 个 i32 元素的数组
[16] [8] i32          // 二维数组（16 行 8 列）
[N] T                 // 泛型数组（编译期确定大小）
```

数组大小必须是编译期可确定的常量表达式。在泛型参数中可以使用数组类型：

```c
Vec[[10] i32]         // Vec 的元素类型是 [10] i32 数组
```

### 2.7 类型后缀

数值字面量可附带类型后缀指定精度：

```c
42i32       // 有符号 32 位整数
255u8       // 无符号 8 位整数
3.14f64     // 64 位浮点数
```

---

## 3. 运算符

### 3.1 前缀一元运算符

```c
!value      // 逻辑非
-value      // 算术取负
+value      // 一元正号
~value      // 按位取反
```

支持链式：`!!x`, `--n`（双重取负即为正）。

### 3.2 后缀一元运算符

Cubec 使用**后缀语法**表示指针操作和 try 操作（区别于 C 的前缀 `*`/`&`）：

```c
value.*     // 解引用（读取指针指向的值）
value.&     // 取地址（获取 value 的指针）
value.?     // Try/unwrap（安全地解包 Result/Option 类型）
```

### 3.3 二元运算符与优先级

运算符按优先级从低到高：

| 优先级 | 运算符 | 结合性 |
|--------|--------|--------|
| 1 | `\|\|` | 左结合 |
| 2 | `&&` | 左结合 |
| 3 | `\|` | 左结合 |
| 4 | `^` | 左结合 |
| 5 | `&` | 左结合 |
| 6 | `==` `!=` `extends` | 左结合 |
| 7 | `<` `>` `<=` `>=` | 左结合 |
| 8 | `<<` `>>` | 左结合 |
| 9 | `+` `-` | 左结合 |
| 10 | `*` `/` `%` | 左结合 |

> **注意**：赋值运算符（`=`、`+=` 等）和逗号运算符（`,`）目前**不作为**二元表达式处理，将在语句层面单独实现。

### 3.4 三元条件运算符

```c
condition ? consequent : alternate
```

支持嵌套：`a ? b ? c : d : e`，解析规则为右结合。

### 3.5 展开运算符

`...` 运算符用于展开参数或结构体字段：

```c
func(...args)          // 在函数调用中展开参数
Type[...type_args]     // 在泛型参数中展开类型
```

`...` 在词法层面被识别为独立的三字符运算符，与 `&&=`、`||=` 同级匹配。

---

## 4. 表达式

### 4.1 分组表达式

```c
(a + b) * c
```

括号用于改变求值顺序。

### 4.2 函数调用

```c
callee(arg1, arg2, ...)
```

- 支持零参数：`foo()`
- 支持展开参数：`fn(...args)`
- 支持链式调用：`foo()()`
- 支持混合命名空间/类型成员访问：`obj.method()`, `foo().field`, `Vec::create()`

### 4.3 成员访问

```c
host.field
host.nested.field       // 链式实例成员访问
```

- `.` 仅用于对象/变量的**实例成员**访问（字段和方法）

### 4.4 命名空间/类型成员访问

```c
std::vec::Vec           // 命名空间导航
File::open("a.txt","r") // 类型静态方法调用
Vec::new()              // 类型静态方法调用
```

- `::` 仅用于**类型级**访问：命名空间导航和类型静态成员/方法
- 类型表达式中使用 `::`：`*std::vec::Vec` → 指向 `std::vec::Vec` 的指针
- 混合使用：`std::Vec::new().field` → `::` 访问静态方法，`.` 访问实例字段

### 4.5 泛型实例化

使用方括号语法表示泛型参数：

```c
Vec[i32]                // 单参数
Map[string, i32]        // 多参数
fn[T](arg)              // 函数泛型调用
fn[a][b]                // 链式实例化
```

> 与切片表达式的歧义通过 `:` 消解：`arr[0:10]` 是切片，`arr[0]` 是泛型实例化。单参数形式的最终语义留待语义分析阶段确定。

### 4.6 切片表达式

```c
host[start:length]      // 完整切片
host[start:]            // 省略 length（截取到末尾）
host[:length]           // 省略 start（从开头截取）
arr[0:1][1:2]           // 链式切片
```

### 4.7 typeof 表达式

`typeof` 是编译期类型计算表达式，用于在类型注解中获取一个表达式的类型，而不实际执行该表达式：

```c
typeof(x)              // 获取变量 x 的类型
typeof(a + b)          // 获取表达式 a + b 的结果类型
typeof(foo())          // 获取函数返回类型
```

`typeof` 在语法上被视为**顶层类型表达式**（与三元类型同级），可以用于任何需要类型的地方。支持后缀命名空间访问和泛型实例化，也可以作为指针/切片/数组的基础类型：

```c
typeof(File)::open     // typeof 结果 + 命名空间访问（类型静态方法）
typeof(Vec)[i32]       // typeof 结果的泛型实例化
* typeof(x)            // 指向 typeof(x) 结果类型的指针
[] typeof(x)           // typeof(x) 结果类型的切片
```

> **注意**：`typeof` 仅在编译期计算类型，内部表达式不会在运行时求值。

### 4.8 初始化列表

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

> **错误**：unclosed brace（`.{1`）、混用字段与位置模式（`.{1, .x = 2}`）会触发解析错误。trailing comma（`.{1, }`）是允许的。

### 4.9 分组类型表达式

用于在类型注解中明确分组复杂类型，阻止复合类型的贪婪消费行为：

```c
([]i32)              // 分组切片类型
(*i32)               // 分组指针类型
(Vec[i32])           // 分组泛型类型
(Ptr[T])             // 分组泛型指针类型
(const a) ? b : c    // 阻止 const 贪婪消费三元
(*a) ? b : c         // 阻止指针贪婪消费三元
(func(i32) -> i32) ? Vec[i32] : f32  // 阻止函数类型贪婪消费三元
```

分组类型表达式使用括号将类型表达式包装，与值级分组表达式共用 `( expr )` 语法。类型和值表达式已统一解析路径，括号内使用 `read_expression()` 解析。

### 4.10 类型级三元条件表达式

用于类型注解中的编译期类型分支选择。condition 支持三种形式：

- **类型约束**：`T extends U`、`T == U`、`T != U`（见 4.11 节）
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

类型级三元与值级三元共用 `? :` 语法，但语义不同：类型级三元在编译期类型推导时求值，用于泛型约束中的类型分支选择。复合类型（指针/切片/数组/const/volatile/函数类型）贪婪消费内部类型表达式，包括三元：

```c
* a ? b : c           // → *(a ? b : c)   指针贪婪消费三元
[] a ? b : c          // → [](a ? b : c)  切片贪婪消费三元
[10] a ? b : c        // → [10](a ? b : c) 数组贪婪消费三元
const a ? b : c       // → const(a ? b : c) const 贪婪消费三元
func(i32) -> A ? B : C  // → func(i32) -> (A ? B : C) 返回类型贪婪消费三元
```

需要消歧时用分组阻止贪婪：

```c
(*a) ? b : c          // → ternary(pointer(a), b, c)
(const a) ? b : c     // → ternary(const(a), b, c)
(func(i32) -> i32) ? Vec[i32] : f32  // → ternary(func(i32) -> i32, Vec[i32], f32)
```

条件表达式 `a ? : b`（缺少 consequent）和 `a ? b :`（缺少 alternate）会触发解析错误。

### 4.11 类型约束表达式

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

`extends`、`==` 和 `!=` 都是二元运算符（优先级 6，同级左结合），在 `read_expression_binary` 中统一解析。`T extends U ? X : Y` 被正确解析为 `(T extends U) ? X : Y`，因为 `extends` 优先级高于 `? :`。

除了类型约束，condition 还可以是括号包裹的编译期表达式（如 `(a + b) ? X : Y`）或简单类型标识符。

> **注意**：独立的类型约束表达式（如 `T extends U` 不带 `?`）在类型表达式中不合法——它们只在类型三元条件或泛型定义上下文中有效。在值上下文中，`(typeof(a) extends i32) ? 1 : 2` 是合法的——typeof+extends 作为分组条件。

---

## 5. 语句

### 5.1 变量声明

```c
var x = 42;                    // 自动类型推导
var x: i32 = 42;               // 显式类型注解
var a = 1, b = 2, c = 3;       // 多变量声明
var vec = .Vec{1, 2, 3};       // 初始化列表作为值
var point = .Point{.x = 1, .y = 2};  // 带字段的初始化列表
```

### 5.2 条件语句

```c
if(condition) {
    // ...
} else if(other) {
    // ...
} else {
    // ...
}
```

### 5.3 循环语句

```c
// for 循环 — C 风格三段式
for(var i = 0; i < 10; i = i + 1) {
    // ...
}

// foreach 循环 — 迭代器遍历
// lvalue 模式：使用已有变量
foreach(item of collection) {
    // ...
}
// var 模式：声明新的可变循环变量
foreach(var item of collection) {
    // ...
}
// var + 类型注解
foreach(var item: i32 of collection) {
    // ...
}

// while 循环
while(condition) {
    // ...
}

// do-while 循环
do {
    // ...
} while(condition);
```

### 5.4 switch 语句

```c
switch(value) {
    case(1) -> { /* ... */ }
    case(2, 3) -> { /* ... */ }
    else -> { /* ... */ }
}
```

> switch 仅作为语句使用，不是表达式。

### 5.5 跳转语句

```c
break;
continue;
return value;
```

### 5.6 defer 语句

```c
defer cleanup();        // 表达式形式：在当前作用域退出时执行
defer {                 // 块形式：在当前作用域退出时执行
    file.close();
}
```

### 5.7 块语句

```c
{                   // 块语句：引入新作用域
    var x: i32 = 1;
    foo();
}

{}                  // 空块语句
{ { ; } }           // 嵌套块
```

块语句由一对花括号包裹的语句序列组成，用于引入新的作用域。块可以为空。

### 5.8 表达式语句

```c
some_function();       // 函数调用语句
1 + 2;                 // 表达式求值语句（结果被丢弃）
std::Vec::create();    // 命名空间静态方法调用语句
```

表达式语句由一个表达式后跟分号 `;` 组成。缺少分号是语法错误。

---

## 6. 声明

### 6.1 函数声明

```c
func add(a: i32, b: i32): i32 {
    return a + b;
}

// 泛型函数
func identity[T](value: T): T {
    return value;
}
```

函数返回类型使用 `:` 声明（非 `->`）。函数类型表达式使用 `->` 表示返回类型。

### 6.2 函数类型表达式

```c
func(i32, i32) -> i32          // 函数类型
func(*void, u64) -> i32        // 带指针参数
```

### 6.3 结构体声明

```c
struct Point {
    x: f64;
    y: f64;
}

// 泛型结构体
struct Vec[T] {
    data: *T;
    len: u64;
}
```

### 6.4 枚举声明

TypeScript 风格枚举，编译期常量。成员可指定类型和值，均可省略。不支持泛型（泛型需求由 union 承担）。

```c
// 简单枚举（类型和值均省略，默认自增）
enum Color { Red, Green, Blue }

// 带类型注解
enum Status { Ok: u8, Error: u8 }

// 带类型和值
enum Color { Red: u8 = 0, Green: u8 = 1, Blue: u8 = 2 }

// 仅带值
enum Color { Red = 0, Green = 1, Blue = 2 }

// 导出
export enum Color { Red, Green, Blue }

// 匿名 enum 类型表达式
type Flags = enum { A: u8 = 1, B: u8 = 2 }
```

### 6.5 联合体声明

Rust 风格 tagged union，支持泛型。字段用逗号分隔。

```c
// 简单联合体
union Option[T] { value: T, tag: u64 }

// 导出
export union Result[T, E] { value: T, err: E }

// 匿名 union 类型表达式
type Res = union { ok: i32, err: *u8 }
```

### 6.6 cunion 声明

C 风格联合体，兼容 C 语言。字段用分号分隔，无 tag 字节。不支持泛型、export、匿名类型表达式。

```c
cunion Data { int_val: i32; float_val: f64; }
```

### 6.7 interface 声明

interface 是语法上的类型，但语义上不是真正的类型——没有编译产物，仅用于泛型约束（见 `10-union-interface.md`）。

```c
interface Printable {
    func to_string(self: *Self): string
}

// 泛型 interface
interface Container[T] {
    len(self): u64
    get(self, idx: u64): T
}
```

---

## 7. 修饰符

### 7.1 修饰符总览

| 修饰符 | 作用域 | 修饰目标 |
|--------|--------|---------|
| `builtin` | 声明级 | type / var / func |
| `extern` | 声明级 | func / var |
| `register` | 声明级 | var |
| `comptime` | 声明级 + 语句 | var / func / if / for / block |
| `inline` | 声明级 | func |
| `export` | 声明级 | func / type / var |
| `pub` | 字段级 | struct field |

### 7.2 互斥矩阵

`builtin`、`extern`、`register`、`comptime` 四者互斥，声明只能选其一。`inline` 必须有函数体，与 `builtin`（无体）和 `extern`（无体/外部体）天然互斥。`inline` + `comptime`：comptime 下忽略 inline。`export` 正交于所有其他修饰符。`pub` 仅修饰 struct 字段，与 `export` 职责不同。

详见 `09-modifiers.md`。
