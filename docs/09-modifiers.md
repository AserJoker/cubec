# Cubec 修饰符体系

## 1. 总览

Cubec 有 7 个修饰符，分为三类：

| 修饰符 | 作用域 | 修饰目标 |
|--------|--------|---------|
| `builtin` | 声明级 | type / var / func |
| `extern` | 声明级 | func / var |
| `comptime` | 声明级 + 语句 | var / func / if / for / block |
| `inline` | 声明级 | func |
| `export` | 声明级 | func / type / var |
| `pub` | 字段级 | struct field |
| `using` | 声明级 | var |

---

## 2. 互斥矩阵

### 2.1 四选一修饰符

`builtin`、`extern`、`comptime`、`using` 四者互斥，声明只能选其一：

```
builtin  × extern  × comptime  × using
```

| 组合 | 结果 |
|------|------|
| builtin + extern | ✗ 互斥 |
| builtin + comptime | ✗ 互斥 |
| builtin + using | ✗ 互斥 |
| extern + comptime | ✗ 互斥 |
| extern + using | ✗ 互斥 |
| comptime + using | ✗ 互斥 |

### 2.2 inline 规则

- `inline` **必须有函数体**，与 `builtin`（无体）和 `extern`（无体/外部体）天然互斥
- `inline` + `comptime`：comptime 下忽略 inline（编译期求值无需内联）

### 2.3 export 规则

`export` 正交于所有其他修饰符，可自由组合：

```c
export builtin func panic(): void
export comptime var N = 10
export inline func add(a: i32, b: i32): i32 { return a + b; }
export using var f: File = .{};
```

### 2.4 pub 规则

`pub` 仅修饰 struct 字段，与 `export` 职责不同，不冲突。

---

## 10. using

RAII 风格变量声明修饰符，在作用域退出时自动调用 `__dispose__` 方法：

```c
using f:File = File.open("data.txt");
// 等价于 var f:File = File.open("data.txt"); defer |f| { f.__dispose__(); }
```

- 声明的类型**必须实现** `__dispose__` 方法（返回类型必须为 `void`）
- **不允许**在模块作用域使用（无 `defer` 语义）
- **不允许**使用 `undefined` 初始化
- 与 `builtin`、`extern`、`comptime` 互斥
- 与 `export` 正交（`export using` 允许）
- `using` 是变量声明修饰符，不是独立语句

---

## 3. builtin

声明编译器内建接口，由编译器提供实现：

```c
builtin type RemoveConst[T]              // 内建类型变换
builtin var VERSION: const string        // 内建编译期常量
builtin func panic(): void               // 内建函数
builtin func cast[T, K](value: *K): *T   // 指针强制转换
builtin func packed[T](): T              // packed 类型变换
builtin func align[N, T](): T            // align 类型变换
```

- **无函数体/初始值** — 实现由编译器提供
- 可修饰 `type`、`var`、`func` 三种声明
- 与 `extern`、`comptime`、`inline` 互斥
- 与 `export` 正交

---

## 4. extern

标记声明由外部链接提供实现，用于 FFI：

```c
extern func malloc(size: u64): *void
extern func free(ptr: *void): void
extern var errno: i32
```

- **无函数体/初始值** — 由链接器解析符号
- 不可在 comptime 中调用（不可预测的副作用/IO）
- extern 变量属于全局，地址在链接期解析
- 与 `builtin`、`comptime`、`inline` 互斥
- 与 `export` 正交

---

## 5. comptime

编译期求值修饰符和语句（详见 `07-comptime.md`）：

```c
comptime var N = 10;                    // 编译期变量
comptime func factorial(n: i32): i32    // 编译期函数
comptime if (N > 5) { ... }            // 编译期条件
comptime for (...) { ... }             // 编译期循环
comptime { ... }                       // 编译期执行块
```

- 与 `builtin`、`extern` 互斥
- comptime func 不生成运行时代码
- comptime 下忽略 `inline`

---

## 6. inline

强制内联（不是建议），函数体在调用处展开：

```c
inline func add(a: i32, b: i32): i32 { return a + b; }
```

- **必须有函数体** — 无体的 builtin/extern 不可使用 inline
- 与 `extern`、`builtin` 互斥（无体天然互斥）
- comptime 下忽略 inline（编译期求值无需内联，不报错）
- 与 `export` 正交
- C 后端映射为 `static inline`

---

## 7. export

模块级导出修饰符，控制跨模块可见性（详见 `06-modules.md`）：

```c
export func add(a: i32, b: i32): i32 { return a + b; }
export type Point = struct { x: i32; y: i32; }
export var VERSION: const string = "1.0";
```

- 正交于所有其他修饰符
- 未 export 的符号仅在当前模块内可见
- C 后端中 export 符号不加 `static`

---

## 8. pub

仅修饰 struct 字段可见性：

```c
type Point = struct {
    pub x: i32;     // pub 字段，跨模块可访问
    pub y: i32;
    _id: u64;       // 非 pub 字段，模块内可访问
}
```

- `pub` 标记字段 `is_pub = true`
- 与 `export` 职责不同：
  - `pub` — 字段级，控制 struct 字段跨模块访问
  - `export` — 模块级，控制声明跨模块可见性
- 访问控制框架已就位：`_is_field_accessible()` 在字段访问时检查可见性
- 当前阶段（模块系统未实现）：所有字段在当前模块内均可访问
- Phase 8 实现模块系统后：非 pub 字段从其他模块访问时报错
