# Cubec Union 与 Interface 语义

## 1. Union 语义

### 1.1 运行时表示

union 在运行时是 **tagged union** — 带类型标签的联合体：

```c
union Result[E, T] { value: T; error: E; }
// 运行时等价于：
// struct { __type__: size_t, data: cunion { value: T, error: E } }
```

- `__type__` 存储当前活跃变体的类型 hash（size_t）
- `data` 是 cunion，所有字段 offset=0，size=max(fields.size)
- `__` 前缀字段保留给编译器，用户不允许定义

### 1.2 unionIs 类型检查

`unionIs[T, U](u: U): bool` 检查 `u.__type__ == T的hash`：

```c
var result = Result[string, i32]{ value: 42 };
if (unionIs[i32](result)) {
    // result 当前持有 i32 变体
    var v = result.value;  // 安全访问
}
```

### 1.3 .? 错误传播

`.?` 操作符检查 union 当前变体是否匹配"值变体"（第一个字段），不匹配则传播错误：

```c
union Result { value: i32; err: string; }
var r = .Result{.value = 42};
var v = r.?;           // OK: tag 匹配 value，v = 42

var r2 = .Result{.err = "fail"};
var v2 = r2.?;         // 编译错误：union is in error state
```

等价语义（GCC statement expression 形式）：
```
a.b.? ≡ ({ typeof(a.b) res = a.b; if(res.isError()) return <当前函数类型>::ofError(res.error()); else res.value() })
```

- 对 union：检查 `__type__` 是否匹配第一个字段类型，匹配则返回其值，否则传播错误
- 对指针：等同于解引用（null 时报错）
- 在函数中，`.?` 触发错误返回（类似 try-catch 传播）
- 在 comptime 中，`.?` 触发编译错误

### 1.4 .! 断言解包

`.!` 操作符与 `.?` 类似，但失败时直接 panic 而非传播错误：

```c
union Result { value: i32; err: string; }
var r = .Result{.value = 42};
var v = r.!;           // OK: tag 匹配 value，v = 42

var r2 = .Result{.err = "fail"};
var v2 = r2.!;         // panic: union is not in the expected variant
```

- 对 union：检查 `__type__` 是否匹配第一个字段类型，不匹配则 panic
- 对指针：检查是否为 null，null 则 panic
- 在 comptime 中，panic 表现为编译错误

### 1.5 union 成员

union 支持与 struct 相同的成员类型：

- 字段：`<name> : <type> ;` — 变体字段，存入 cunion data
- 方法：`func <name> ...` — 实例/静态方法
- 静态字段：`var <name> [: <type>] = <expr> ;`
- 关联类型：`type <name> ... ;`
- spread：`...<expr> ;`
- 嵌套声明

### 1.6 cunion

cunion 是 **C 风格 untagged union**，与 C union 完全一致：

```c
cunion Data { i32Val: i32; f64Val: f64; }
```

- 所有字段 offset=0，size=max(fields.size)
- 无 `__type__` 标签
- 无类型安全检查
- 用于与 C FFI 交互或手动管理类型标签的场景

| 类型 | 标签 | 类型安全 | 用途 |
|------|------|---------|------|
| union | 有（`__type__`） | 有 | 通用 tagged union |
| cunion | 无 | 无 | C 兼容 / 手动管理 |

---

## 2. Interface 语义

interface 是语法上的类型，但语义上**不是真正的类型**：

- **没有编译产物** — 不生成 vtable、不占内存、不产生运行时数据
- **仅用于泛型约束** — 通过 `extends` 检查实际类型是否满足接口的方法签名
- **不可声明变量** — `var x: Printable` 是非法的（大小/布局未知）
- **可用作类型表达式** — `T extends Printable` 合法，`*Printable` 不合法

```c
interface Printable {
    func to_string(self: *Self): string
}

func print[T extends Printable](value: T) {
    var s = value.to_string();
}

type Point = struct {
    x: i32; y: i32;
    func to_string(self: *Point): string { ... }
}
// Point 满足 Printable（有 to_string 方法），可传入 print
```

interface 约束检查流程：
1. 遍历 interface 定义的方法签名列表
2. 在实际类型的 instance_methods 中查找同名方法
3. 检查签名是否结构等价
4. 所有方法都满足 → `T extends Interface` 返回 true

- 约束检查是**结构等价**（方法名+签名匹配），不是名字等价
- 暂不支持多约束（一个泛型参数只能 extends 一个 interface）
