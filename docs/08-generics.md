# Cubec 泛型机制

## 0. 编译器设计原则

### 0.1 统一 template → instances

**所有函数/类型统一为 template → instances 结构**，不区分泛型与非泛型：

- 有泛型参数：按需单态化（遇到 `callee[types]` 时实例化）
- 无泛型参数：名字收集完成后立即单态化（只有一种具体化，即自身）

上一版实现的主要问题就是大量泛型/非泛型路径区分，统一后只有一条路径，"非泛型"只是自动实例化的特例。

### 0.2 函数统一处理

全局函数、结构体/共用体方法、局部函数、表达式内函数语义完全一致，区别仅在符号可见性。定义时注册到当前作用域即可，不按种类做分支。

### 0.3 类型统一处理

struct/union/enum/interface/type alias 等类型声明语义一致，区别仅在符号可见性。定义时注册到当前作用域，不按种类做分支。

### 0.4 逐作用域递归处理

名字收集 → 定义解析 → 实例化 → 函数体解析在每个作用域内都执行一遍。局部作用域内也可以有函数、类型等声明。

## 1. 单态化（Monomorphization）

泛型不是运行时多态，是编译期单态化：

```
identity[i32] → 生成一份 i32 版本的函数体
identity[f64] → 生成一份 f64 版本的函数体
```

---

## 2. 约束检查（extends）

泛型约束使用 `extends` 关键字：

```c
func print[T extends Printable](value: T) {
    value.to_string()
}
```

**`extends` 是二元运算符**（优先级6），在类型表达式上下文中求值：

```c
T extends Printable       // 返回 bool：T 是否满足 Printable 约束
T extends Numeric         // T 是否满足 Numeric 约束
```

检查流程：
1. 解析 T 的实际类型
2. 遍历约束接口的方法列表
3. 对每个方法，在实际类型的 instance_methods 中查找同名方法
4. 检查签名是否结构等价
5. 所有方法都满足 → extends 返回 true → 实例化成功

**`?` 通配符** — 在泛型约束中用作全部/局部的通配符：

```c
// 无约束泛型 — T 可以是任意类型
func identity[T](x: T): T { return x; }

// 带约束泛型 — T 必须满足约束
func print[T extends Printable](value: T) { value.to_string(); }

// ? 通配 — T 是一个 slice，但不关心具体元素类型
func sum[T extends []?](data: T): i32 { ... }

// ? 通配 — T 是一个指针，但不关心指向类型
func is_null[T extends *?](ptr: T): bool { return ptr == nil; }
```

- `[T]` — T 无约束，可以是任意类型
- `[T extends Printable]` — T 必须满足 Printable 约束
- `[T extends Printable & Serializable]` — T 必须同时满足所有约束（AND 语义，`&` 连接）
- `[T extends []?]` — T 必须是 slice 类型，元素类型任意
- `[T extends *?]` — T 必须是指针类型，指向类型任意
- `[T extends [?]?]` — T 必须是数组类型，长度和元素类型均任意
- `?` 可匹配类型参数和值参数（如数组长度），出现在 `extends` 约束的类型模式中
- `?` 不能单独作为类型使用，只能用于约束模式
- 为可读性，复杂模式应封装为类型别名：`type Array[T, N: u64] = [N]T`
- 多约束使用 `&` 连接：`[T extends A & B]` 表示 T 必须同时满足 A 和 B

---

## 3. 实例化缓存

用 `(func_name, type_arg_hashes)` 作为缓存 key，同一组具体类型参数只实例化一次。

---

## 4. 参数包（Parameter Packs）

Cubec 支持泛型参数包（类似 C++ variadic templates / Rust 参数包），用于定义接受可变数量类型参数的泛型函数和类型。

### 4.1 参数包定义

在泛型参数列表中使用 `...` 前缀定义参数包。参数包**必须是泛型参数列表的最后一个参数**：

```c
// 仅含参数包
func foo[...Args](): void {}

// 普通参数 + 参数包
func foo[T, ...Args](x: T, ...args: Args): void {}

// 参数包带约束 — 展开后的每个类型都必须满足约束
func sum[...Args extends i32](...args: Args): i32 { ... }
```

**规则**：
- 参数包只能出现一次，且必须在泛型参数列表末尾
- `...Args, T` 是非法的（参数包后不可有普通参数）
- `...A, ...B` 是非法的（不允许出现多个参数包）
- 参数包可带 `extends` 约束，每个展开的类型都必须满足该约束

### 4.2 函数参数中的包展开

在函数参数列表中使用 `...args: PackName` 语法声明与参数包对应的可变参数：

```c
func foo[T, ...Args](x: T, ...args: Args): void {}

foo(1);           // Args = [] (空展开), args = ()
foo(1, 2, 3);     // Args = [i32, i32], args = (2, 3)
foo(1, "a", 3.0); // Args = [string, f64], args = ("a", 3.0)
```

- `...args: Args` 表示 args 对应参数包 `Args` 中的类型
- 参数包展开为零个或多个参数
- 当参数包为空展开时，对应的函数参数位置消失

### 4.3 函数类型中的包展开

参数包可用于函数类型表达式，实现高阶函数模式：

```c
func wrap[R, ...Args](fn: func(...Args) -> R): func(...Args) -> R {
    return func |fn| (...args: Args): R {
        return fn(...args);
    };
}
```

- `func(...Args) -> R` 是一个函数类型，其参数由参数包 `Args` 展开
- 参数包在函数类型参数列表中的位置同样支持空展开

### 4.4 调用参数中的包展开

使用 `...expr` 语法在函数调用中展开参数包：

```c
func apply[R, ...Args](fn: func(...Args) -> R, ...args: Args): R {
    return fn(...args);
}
```

- `fn(...args)` 将 args 展开为独立的调用参数
- 与 `...` 展开运算符统一（见 `04-expressions.md` 第6节）

### 4.5 空包展开

当参数包没有任何类型实参时，包展开位置产生空序列：

```c
func foo[...Args](): void {}
foo[]();  // OK: Args 为空，调用等价于 foo()
```

- `foo[]()` 语法：空的泛型实参列表表示参数包零展开
- 空展开在函数参数位置消失，在函数类型参数位置消失

### 4.6 装饰器模式

参数包使得类型安全的装饰器/包装器模式成为可能：

```c
func wrap[R, ...Args](fn: func(...Args) -> R): func(...Args) -> R {
    return func |fn| (...args: Args): R {
        // 前置逻辑
        var result = fn(...args);
        // 后置逻辑
        return result;
    };
}
```

此模式捕获函数签名中的所有参数类型和返回类型，生成类型安全的包装函数。

### 4.7 语义表示

- **类型层**：参数包类型在实例化时由 VM 类型层表示，含元素类型向量（早期文档中的 `TYPE_GENERIC_PACK` 分类已不存在，见 `02-type-system.md`）
- **值层**：`comptime_value_t` 中 `COMPTIME_VALUE_PACK` 表示参数包值，含 `elements` 向量
- **符号层**：`symbol_t` 的 `generic_param` 中 `is_rest` 标记参数包
- **函数参数**：`cubec_function_argument_t` 中 `is_rest` 标记包展开参数 (`...args`)
