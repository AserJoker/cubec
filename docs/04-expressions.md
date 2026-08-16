# Cubec 表达式语义

## 1. 禁止丢弃返回值

有返回值的表达式**必须被使用**，不允许静默丢弃：

```c
foo();            // Error: foo 返回 i32，值未使用
_ = foo();        // OK: 显式丢弃
var x = foo();    // OK: 赋值使用
if (foo() > 0) {} // OK: 条件中使用
```

- 丢弃返回值必须用 `_ = expr;` 显式标记
- `_` 是保留标识符，不可用作变量名、参数名、字段名等
- void 返回值的函数调用不受此限制
- 适用于所有表达式上下文（函数调用、二元运算、成员访问等）

---

## 2. 全局变量初始化约束

参考 C 语言规则，全局变量的初始化值必须是**编译期可计算**的常量表达式：

```c
var N: i32 = 42;                  // OK: 字面量
var PTR: *i32 = &global_var;      // OK: 全局变量地址（链接期解析）
var FN: func(): void = my_func;   // OK: 函数地址（链接期解析）
var X: i32 = some_func();         // Error: 运行时函数调用
```

**编译期可计算**不要求完全在编译期求值，而是指表达式在语义上是常量：

| 表达式 | 是否合法 | 原因 |
|--------|---------|------|
| 字面量 | ✓ | 编译期已知 |
| 枚举项 | ✓ | 编译期已知 |
| `sizeof` / `alignof` | ✓ | 编译期已知 |
| 全局变量地址 `x.&` | ✓ | 链接期解析，语义上是常量 |
| 函数地址 `func_name` | ✓ | 链接期解析，语义上是常量 |
| 偏移计算 `offsetof` | ✓ | 编译期已知 |
| 运行时函数调用 | ✗ | 不可预测 |
| 局部变量引用 | ✗ | 不在全局作用域 |
| `comptime` 函数调用 | ✓ | 编译期求值 |

---

## 3. 成员调用 desugaring

`a.method(args)` 是语法糖，编译器将其脱糖为静态方法调用：

```
obj.method(args) → typeof(obj)::method(&obj, args)
```

### 3.1 对象调用

```c
var p = .Point{ .x = 1, .y = 2 };
p.toString()       // → Point::toString(&p)
```

编译器自动取 `&obj` 作为 `self` 参数。

### 3.2 指针调用（auto-deref）

```c
var pp = p.&;
pp.toString()      // → Point::toString(pp)，pp 已是指针，直接传递
```

指针 host 不需要额外取地址，直接作为 `self` 参数传递。

### 3.3 self 接收规则

- `self` 参数始终是指针（`*StructType`）
- 方法签名中第一个参数如果是指向 receiver 类型的指针，即为 `self` 参数
- 用户调用时不需要传递 `self`，编译器自动插入

---

## 4. 匿名函数与闭包

### 4.1 语法

```c
// 匿名函数
func |captures| (params): ReturnType { body }

// 命名局部函数（支持闭包捕获）
func |captures| name(params): ReturnType { body }
```

- capture 列表用 `|` 包裹，标识符 only（不允许表达式初始化）
- 无捕获时 `||` 可省略：`func () { }` 等价于 `func || () { }`
- 支持命名局部函数：`func |x| test(): void { x = x + 1; }`
- 捕获列表可在函数名之前，用于匿名和命名函数

### 4.2 捕获语义

**运行时与 comptime：均按值捕获**。闭包内修改不影响原变量：

```c
var x = 1;
var f = func |x| { x = 2; };  // 捕获 x 的副本
f();
// x 仍为 1
```

需要观察修改时，显式捕获指针：

```c
var y = 1;
var yp = y.&;
var g = func |yp| { yp.* = 2; };  // 捕获指针副本
g();
// y 现为 2
```

### 4.3 按值捕获实现

闭包创建时：
1. 遍历捕获列表，获取每个捕获的标识符名
2. 从当前环境查找变量的 comptime_value
3. 克隆值（deep copy）到新创建的隔离环境
4. 隔离环境的 parent 为 NULL，确保与原环境隔离

闭包调用时：
1. 创建调用环境，以捕获的隔离环境为 parent
2. 将参数绑定到调用环境（通过 parent 链访问捕获值）
3. 执行函数体
4. 调用完成后销毁调用环境

调用栈深度限制：`COMPTIME_MAX_CALL_STACK_DEPTH`

---

## 5. lvalue 与 const 检查

### 5.1 lvalue 判断

以下表达式是 lvalue（可赋值）：

- 标识符（变量名）
- 成员访问（`expr.field`）
- 解引用（`expr.*`）
- 元组下标（`expr[const_index]`，当 expr 为 tuple/generic_instance 时）
- 泛型实例化（`expr[index]`，如 `__get__` 返回 lvalue）

### 5.2 const lvalue 检查

对 const 限定的 lvalue 赋值报错 "cannot assign to const-qualified expression"：

| 场景 | 检查规则 |
|------|---------|
| 标识符 `x = val` | `type_is_const(x.type)`（由 `type_t.mut` 表达） |
| 成员 `p.x = val` | host 类型是 const → 字段不可写；字段类型本身是 const → 也不可写 |
| 解引用 `p.* = val` | pointee 类型是 const → 不可通过指针写入 |

---

## 6. 展开运算符 `...`

`...` 在 Cubec 中有三个不同用途：

### 6.1 struct/union/cunion 中复用类型

在类型定义中使用 `...Type` 展开另一个类型的字段和方法，**不建立类型关系**，纯粹的复用：

```c
type Base = struct { x: i32; y: i32; }
type Extended = struct {
    ...Base          // 复用 x, y 字段和 Base 的方法
    z: i32
}
// Extended 与 Base 无继承关系
// 但 *Extended 可退化为 *Base（前2个字段结构等价）
```

### 6.2 泛型参数包

在泛型定义中使用 `...T` 定义参数包（类似 C++ variadic templates），详见 `08-generics.md` 第4节。

### 6.3 函数调用和初始化展开

在函数调用和初始化语句中，`...expr` 语法糖展开对象或数组（类似 TypeScript），但要求**编译期确定数量和字段**：

```c
var args = .{ 1, 2, 3 };
foo(...args);      // 编译期已知 args 有3个元素 → foo(1, 2, 3)

var base = .Base{ .x = 1, .y = 2 };
var ext = .Extended{ ...base, .z = 3 };  // 展开 base 的字段
```

**包展开调用** — 当 `...expr` 中的 expr 是参数包变量时，展开为对应的多个调用参数：

```c
func apply[R, ...Args](fn: func(...Args) -> R, ...args: Args): R {
    return fn(...args);    // args 展开为多个参数
}
```

详见 `08-generics.md` 第4节。

---

## 7. AST 节点语义确认

### 7.1 逗号表达式（EXPRESSION_COMMA）

C 风格逗号表达式：从左到右求值，返回最右值。

```c
// Cubec: a, b, c
// 求值 a，再求值 b，再求值 c，整个表达式值为 c
// C 映射: (a, b, c)
```

- 优先级最低（低于赋值）
- 右结合：`a, b, c` → `comma(a, comma(b, c))`
- 仅用于 for 循环初始化/更新等特殊场景
- 逗号在函数参数、初始化列表中仅作分隔符，不构成逗号表达式

### 7.2 取地址运算符（EXPRESSION_ADDR）

语法：后缀 `.&`，操作数**必须是左值**。

```c
// 合法：
var x: i32 = 42;
var p: *i32 = x.&;     // &x

// 非法：
var p: *i32 = (x + 1).&;  // 临时值不可取地址
```

- 左值包括：变量、字段访问、解引用结果、下标访问结果
- 临时值（算术结果、函数返回值）不可取地址
- 结果类型为 `*T`（T 为操作数类型）
