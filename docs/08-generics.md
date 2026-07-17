# Cubec 泛型机制

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
- `[T extends []?]` — T 必须是 slice 类型，元素类型任意
- `[T extends *?]` — T 必须是指针类型，指向类型任意
- `[T extends [?]?]` — T 必须是数组类型，长度和元素类型均任意
- `?` 可匹配类型参数和值参数（如数组长度），出现在 `extends` 约束的类型模式中
- `?` 不能单独作为类型使用，只能用于约束模式
- 为可读性，复杂模式应封装为类型别名：`type Array[T, N: u64] = [N]T`
- 暂不支持多约束

---

## 3. 实例化缓存

用 `(func_name, type_arg_hashes)` 作为缓存 key，同一组具体类型参数只实例化一次。

---

## 4. 参数包

Cubec 支持泛型参数包（类似 C++ variadic templates），使用 `...T` 语法定义：

```c
func variadic[...T](args: ...T): void {
    // args 是编译期已知的参数包
}
```

- `...T` 在泛型参数列表中定义参数包
- 参数包在编译期展开，生成具体化的函数
- 与 `...` 展开运算符的其他用途统一（见 `04-expressions.md` 第6节）
