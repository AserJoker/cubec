# Cubec 代码生成

## 1. 渐进式多后端

Cubec 采用渐进式多后端策略，优先实现 C 后端，后续扩展 LLVM 后端。

| 后端 | 优先级 | 优势 | 劣势 |
|------|--------|------|------|
| C | 第一 | 最简单，复用 C 编译器工具链，快速验证语义 | 依赖外部 C 编译器 |
| LLVM | 第二 | 利用 LLVM 优化和生态 | 依赖 LLVM |

---

## 2. C 后端目标

输出 `.c` 文件，由 C 编译器（clang/gcc）编译为最终产物。

关键映射方向：

- **struct → C struct**：字段布局一致，内存对齐规则与 C 兼容
- **union → tagged struct**：`struct { size_t __type__; cunion { ... } data; }`
- **cunion → C union**：直接映射
- **enum → C enum**：底层类型映射
- **泛型单态化**：每个实例生成一个 C 函数/结构体
- **comptime**：编译期执行完毕，不生成 C 代码
- **defer** → cleanup + goto：生成清理代码块和跳转标签
- **allocator 透传**：显式分配器参数传递给每个函数
- **interface**：无编译产物，仅约束检查
- **builtin func**：映射到编译器内建函数或标准库实现
- **extern**：直接声明为 C extern，由链接器解析

后端策略：C 后端优先，后续扩展 LLVM。C 后端允许 GCC/Clang 扩展，不追求严格标准 C 合规。

---

## 3. 设计原则

1. **C 兼容内存布局** — struct/union 字段布局与 C 一致，支持 extern FFI
2. **单态化输出** — 泛型实例化后每个特化生成独立 C 函数/类型
3. **comptime 消除** — 编译期计算完毕，不输出任何 C 代码
4. **allocator 用户态** — allocator 由用户显式传递，编译器不自动注入参数
5. **名称修饰** — 避免与 C 标准库冲突，使用 `cubec_` 前缀
6. **标准库编码风格** — 类型 UpperCamelCase（`StringBuilder`），字段/方法 lowerCamelCase（`toString`、`nextValue`）

---

## 4. 类型映射

### 4.1 基本类型

| Cubec | C |
|-------|---|
| `i8` | `int8_t` |
| `i16` | `int16_t` |
| `i32` | `int32_t` |
| `i64` | `int64_t` |
| `u8` | `uint8_t` |
| `u16` | `uint16_t` |
| `u32` | `uint32_t` |
| `u64` | `uint64_t` |
| `f16` | `_Float16` |
| `f32` | `float` |
| `f64` | `double` |
| `bool` | `bool`（`<stdbool.h>`） |
| `void` | `void` |
| `string` | 双策略：编译期 `const char*`，运行期 `struct { uint8_t* data; size_t len; size_t cap; }`；编译期字符串可隐式转为 `const []u8` |

### 4.2 指针类型

| Cubec | C |
|-------|---|
| `*T` | `T*` |
| `*const T` | `const T*` |
| `*volatile T` | `volatile T*` |

### 4.3 Slice 类型

```c
// Cubec: []T
// C:
typedef struct {
    T* data;
    size_t len;
} m3a7_slice_T;
```

### 4.4 数组类型

```c
// Cubec: [N]T
// C:
T name[N];
```

### 4.5 struct 映射

```c
// Cubec:
struct Point { x: i32; y: i32; }

// C:
typedef struct {
    int32_t x;
    int32_t y;
} m3a7_Point;
```

- 字段按声明顺序排列，与 C 内存布局一致
- `pub` 字段不影响 C 输出（C 无可见性控制）
- 静态字段 → 翻译为文件作用域变量 + 命名修饰
- 方法 → 翻译为独立函数，首参数为 `self` 指针

### 4.6 union 映射（tagged union）

```c
// Cubec:
union Result[E, T] { value: T; error: E; }

// C:
typedef struct {
    size_t __type__;       // 类型 hash
    union {
        T value;
        E error;
    } data;
} m3a7_Result;
```

- `__type__` 字段存储当前活跃变体的类型 hash
- `data` 是 cunion，所有字段 offset=0

### 4.7 cunion 映射

```c
// Cubec:
cunion Data { i32Val: i32; f64Val: f64; }

// C:
typedef union {
    int32_t i32Val;
    double f64Val;
} m3a7_Data;
```

### 4.8 enum 映射

```c
// Cubec:
enum Color: u8 { Red = 0; Green = 1; Blue = 2; }

// C:
typedef enum {
    m3a7_Color_Red = 0,
    m3a7_Color_Green = 1,
    m3a7_Color_Blue = 2,
} m3a7_Color;
```

- 显式指定底层类型 → C enum 底层类型映射
- 无显式类型 → 从首个 item 推导

### 4.9 interface 映射

**不生成任何 C 代码。** interface 仅用于编译期 `extends` 约束检查。

### 4.10 type 别名映射

```c
// Cubec:
type PointRef = *Point

// C:
typedef Point* m3a7_PointRef;
```

- `typeof` 创建新名字 → `typedef` 指向同一实现

### 4.11 函数类型映射

```c
// Cubec: func(i32): i32
// C: int32_t (*)(int32_t)
```

---

## 5. 语句映射

### 5.1 var 声明

```c
// Cubec:
var x: i32 = 42;

// C:
int32_t x = 42;
```

- `const` 修饰 → `const`
- `volatile` 修饰 → `volatile`
- 全局变量初始化必须是编译期可计算常量
- `= undefined` → 不初始化（C 中局部变量不初始化为随机值，全局为零初始化）

### 5.2 func 声明

```c
// Cubec:
func add(a: i32, b: i32): i32 { return a + b; }

// C:
static int32_t m3a7_add(int32_t a, int32_t b) {
    return a + b;
}
```

- allocator 由用户在源码中显式传递，编译器不自动注入
- `export` → 不加 `static`
- 非 export → 加 `static`
- `extern` → `extern` 声明，无 body
- `builtin` → 映射到编译器内建函数或标准库实现
- `inline` → `static inline`
- `comptime` → 不生成 C 代码

### 5.3 方法映射

```c
// Cubec:
struct Point {
    x: i32; y: i32;
    func toString(self: *Point): string { ... }
}

// C:
m3a7_string m3a7_Point_toString(m3a7_Point* self);
```

- 实例方法 → 独立函数，`self` 作为显式参数
- 静态方法（无 self）→ 独立函数，无 self 参数
- 自动解引用：`ptr.method()` → 编译器自动插入解引用

### 5.4 if / while / do-while / for

直接映射为 C 对应语句。

### 5.5 foreach（迭代器接口）

foreach 通过迭代器接口实现，任何有 `next` 方法的类型都可迭代：

```c
// 迭代器 next 方法返回 {done: bool, value: T} 结构
// Cubec:
foreach (item of collection) { ... }

// C: 调用 next 方法
{
    typeof(collection) _iter = collection;
    while (true) {
        m3a7_IteratorResult _r = m3a7_next(&_iter);
        if (_r.done) break;
        typeof(item) item = _r.value;
        ...
    }
}
```

- 迭代器接口：`func next(self: *Self): {done: bool, value: T}`
- `done = true` 表示迭代结束
- 自定义类型实现 `next` 方法即可支持 foreach

### 5.6 switch

统一映射为 if-else 链：

```c
// Cubec:
switch (x) {
    0 => { ... }
    1 => { ... }
    _ => { ... }
}

// C:
if (x == 0) {
    ...
} else if (x == 1) {
    ...
} else {
    ...
}
```

### 5.7 defer（栈链表 + 函数指针）

每个 defer 块生成一个静态函数，使用栈对象链表串联，exit 时逆序遍历执行：

```c
// Cubec:
func foo(): i32 {
    var f = openFile("test.txt");
    defer |f| { closeFile(f); }
    var g = openFile("data.txt");
    defer |g| { closeFile(g); }
    if (error) { return -1; }
    return 0;
}

// C:
typedef struct cubec_defer_entry {
    void (*fn)(void*);
    void* env;
    struct cubec_defer_entry* prev;
} cubec_defer_entry;

static void cubec_defer_1(void* _env) {
    struct { m3a7_File f; }* env = _env;
    m3a7_closeFile(env->f);
}

static void cubec_defer_2(void* _env) {
    struct { m3a7_File g; }* env = _env;
    m3a7_closeFile(env->g);
}

int32_t m3a7_foo() {
    cubec_defer_entry* _defer_chain = NULL;

    m3a7_File f = m3a7_openFile("test.txt");
    struct { m3a7_File f; } _d1_env = { .f = f };
    cubec_defer_entry _d1 = { .fn = cubec_defer_1, .env = &_d1_env, .prev = _defer_chain };
    _defer_chain = &_d1;

    m3a7_File g = m3a7_openFile("data.txt");
    struct { m3a7_File g; } _d2_env = { .g = g };
    cubec_defer_entry _d2 = { .fn = cubec_defer_2, .env = &_d2_env, .prev = _defer_chain };
    _defer_chain = &_d2;

    if (error) {
        return ({
            while (_defer_chain) {
                _defer_chain->fn(_defer_chain->env);
                _defer_chain = _defer_chain->prev;
            }
            -1;
        });
    }
    return ({
        while (_defer_chain) {
            _defer_chain->fn(_defer_chain->env);
            _defer_chain = _defer_chain->prev;
        }
        0;
    });
}
```

- 每个 defer 块 → 一个静态函数 + 栈上环境结构体
- `_defer_chain` 指向链表尾部（最后注册的 defer）
- return/break/continue/panic 时用语句表达式逆序遍历执行
- 捕获变量打包到环境结构体（按值捕获）
- 同一作用域多个 defer 自然逆序（链表尾部先执行）

### 5.8 break / continue

直接映射。若有 defer 作用域，break 需插入中间 cleanup。

### 5.9 comptime / import / test 语句

**不生成任何 C 代码。** 编译期执行完毕后消除。

---

## 6. 表达式映射

### 6.1 字面量

| Cubec | C |
|-------|---|
| `42` | `42` |
| `3.14` | `3.14` |
| `"hello"` | `"hello"` |
| `'a'` | `'a'` |
| `true` / `false` | `true` / `false` |
| `nil` | `NULL` |
| `undefined` | 不初始化 |

### 6.2 二元运算 / 赋值

直接映射。

### 6.3 成员访问 `.`（自动解引用）

```c
// Cubec: p.x （自动解引用）
// 若 p 是 *Point:  C: p->x
// 若 p 是 Point:   C: p.x
```

编译器根据类型决定是否插入解引用。

### 6.4 解引用 `.*` / 取地址 `.&`

```c
// Cubec: ptr.*  →  C: (*ptr)
// Cubec: obj.&  →  C: (&obj)
```

### 6.5 命名空间访问 `::`

```c
// Cubec: Point::new()
// C: m3a7_Point_new()
```

### 6.6 函数调用

```c
// Cubec: foo(1, 2)
// C: m3a7_foo(1, 2)
```

allocator 由用户显式传递，编译器不隐式插入。

### 6.7 泛型实例化

```c
// Cubec: identity[i32](42)
// C: m3a7_identity_i32(42)
```

单态化后函数名包含类型参数。

### 6.8 初始化列表

```c
// Cubec: Point{ x: 1, y: 2 }
// C: (m3a7_Point){ .x = 1, .y = 2 }
```

使用 C99 指定初始化器。

### 6.9 匿名函数

```c
// Cubec: func |x| { return x + 1; }
// C: 生成唯一的静态函数 + 传递函数指针
```

捕获的变量打包为结构体，作为闭包环境传递。

### 6.10 .? 错误传播

使用 GCC 语句表达式（Statement Expressions）实现：

```c
// Cubec: var v = result.value.?;
// C (GCC statement expression):
({
    if (result.__type__ != HASH_i32) {
        goto cubec_error_handler;
    }
    result.data.value;
})
```

- 语句表达式使 .? 可作为表达式嵌入任意上下文
- 错误传播方式由函数的返回类型决定（goto defer / 返回错误值）
- 依赖 GCC/Clang 扩展，C 后端不追求严格标准 C 合规

### 6.11 union 智能类型窄化

类似 TypeScript 的 `is` 类型守卫，编译器追踪控制流中的 union 类型检查：

```c
// Cubec:
if (union_is[i32](result)) {
    var v = result.value;  // 无需检查 — 前文已验证
}

// Cubec: 无前文检查
var v = result.value;  // 生成运行时检查 + panic
```

```c
// C 输出（有前文检查）:
if (result.__type__ == HASH_i32) {
    int32_t v = result.data.value;
}

// C 输出（无前文检查）:
int32_t v = ({
    if (result.__type__ != HASH_i32) {
        cubec_panic("union type mismatch");
    }
    result.data.value;
});
```

- 编译器在语义分析阶段追踪 union 类型窄化状态
- 已通过 `unionIs` / `if` / `.?` 等检查的路径 → 不生成运行时检查
- 未检查的路径 → 生成运行时检查，非法则 panic
- 类型窄化在分支合并时重置（类似 TDZ 追踪）

### 6.12 typeof / sizeof / alignof

- `typeof(expr)` → 编译期求值，不生成 C 代码
- `sizeof(T)` → `sizeof(C_type)`
- `alignof(T)` → `_Alignof(C_type)`

---

## 7. 名称修饰规则

原则：模块隔离用短hash前缀，保留源码原名便于调试，嵌套避免线性增长。

| Cubec | C 名称 | 规则 |
|-------|--------|------|
| 全局函数 `foo` | `m3a7_foo` | 模块hash + `_` + 名字 |
| 方法 `Point.toString` | `m3a7_Point_toString` | 模块hash + `_` + 类型 + `_` + 方法名 |
| 泛型实例 `identity[i32]` | `m3a7_identity_i32` | 模块hash + `_` + 名字 + `_` + 类型参数 |
| 枚举项 `Color.Red` | `m3a7_Color_Red` | 模块hash + `_` + 类型 + `_` + 项名 |
| 结构体类型 `Point` | `m3a7_Point` | 模块hash + `_` + 类型名 |
| 静态字段 `Point.origin` | `m3a7_Point_origin` | 模块hash + `_` + 类型 + `_` + 字段名 |
| 嵌套函数（foo内第2个） | `m3a7_foo__2` | 模块hash + `_` + 父函数 + `__` + 编号 |

- 模块hash：对模块路径计算短hash（4-6字符），如 `m3a7`、`bf01`
- 同模块内名称冲突由编译器保证不存在
- `__` 双下划线分隔嵌套编号，与单下划线 `_` 区分层级

---

## 8. 魔法方法映射

| Cubec | C |
|-------|---|
| `__get__(self, key)` | 拦截 `[]` 索引读 → 生成函数调用 |
| `__set__(self, key, value)` | 拦截 `[]` 索引写 → 生成函数调用 |
| `__value__(self)` | 隐式转换 → 生成函数调用 |
| `__call__(self, args)` | 可调用对象 → 生成函数调用 |
| `__slice__(self, start, len)` | 拦截切片操作 → 生成函数调用 |

---

## 9. 输出文件结构

每个 Cubec 源文件生成一对 C 文件：

```
foo.cubec → foo.c + foo.h
```

- `.h` 文件：类型定义、函数声明（供其他模块 include）
- `.c` 文件：函数实现
- export 的符号放入 `.h`，非 export 的加 `static`
- 模块间依赖通过 include .h + 前向声明实现
