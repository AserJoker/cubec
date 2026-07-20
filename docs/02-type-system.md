# Cubec 类型系统

## 1. 两层类型架构

**名字层**（Name Layer）— 按名称查找，关联方法集和静态字段：

```c
struct type_name_entry {
    const char *name;
    size_t impl_hash;              // → 指向实现层
    vec_t instance_methods;        // 方法签名列表
    vec_t static_methods;
    vec_t static_fields;
    vec_t associated_types;
};
```

**实现层**（Implementation Layer）— 按 hash 去重缓存，只记录字段和布局：

```c
struct type_impl {
    size_t hash;                   // 结构哈希
    enum type_kind kind;
    size_t size;
    size_t alignment;
    bool is_packed;
    size_t explicit_align;
    union { /* kind-specific fields */ };
};
```

---

## 2. 结构等价

使用结构等价（鸭子类型），不是名字等价：

- `type A = struct { x: i32 }` 和 `type B = struct { x: i32 }` 是同一类型
- 结构等价**只看字段**，不计算方法和静态字段
- 用结构哈希缓存加速比较，hash 冲突时递归比较

---

## 3. 类型分类

```c
enum type_kind {
    TYPE_VOID,
    TYPE_BOOL,
    TYPE_I8, TYPE_I16, TYPE_I32, TYPE_I64,
    TYPE_U8, TYPE_U16, TYPE_U32, TYPE_U64,
    TYPE_F32, TYPE_F64,
    TYPE_CHAR,
    TYPE_STRING,
    TYPE_POINTER,       // *T
    TYPE_SLICE,         // []T
    TYPE_ARRAY,         // [N]T
    TYPE_STRUCT,
    TYPE_UNION,
    TYPE_CUNION,
    TYPE_ENUM,
    TYPE_INTERFACE,
    TYPE_FUNCTION,
    TYPE_TYPE,          // type 本身（泛型参数 T: type）
    TYPE_QUALIFIER,     // const T / volatile T
    TYPE_NIL,           // nil 类型
    TYPE_ERROR,         // 错误占位类型
};
```

---

## 4. 隐式类型转换

Cubec 仅允许以下隐式转换：

| 转换 | 方向 | 条件 |
|------|------|------|
| 指针退化 | `*A → *B` | A 首字段结构等价于 B（见 `03-pointer-semantics.md`） |
| nil 转指针 | `nil → *T` | nil 可转为任意指针类型 |
| 整数拓宽 | `i8 → i16 → i32 → i64` | 只允许向更大同符号类型拓宽 |
| 整数拓宽 | `u8 → u16 → u32 → u64` | 只允许向更大同符号类型拓宽 |
| 浮点拓宽 | `f32 → f64` | 只允许向更大浮点类型拓宽 |
| 添加 const | `T → const T` | 添加 const 限定安全，反之不行 |
| 指针 pointee const | `*T → *const T` | 指向类型添加 const 安全，递归检查 |

- **不允许**隐式窄化（`i32 → i8` 需显式 builtin cast）
- **不允许**符号交叉（`u32 → i32` 需显式 builtin cast）
- **不允许**整浮互转（`i32 → f64` 需显式 builtin cast）
- **不允许** `[]T → *T` 或 `[N]T → []T`（需显式操作）
- **不允许** `const T → T`（去掉 const 不安全）

---

## 5. nil 类型

- 全局唯一值 `nil`
- 可隐式转换为任意 `*T`
- 任何类型不可转换为 nil
- nil 不可用于定义变量（和 void 一样）
- 可与任意 `*T` 做相等比较

---

## 6. 魔法方法

结构体支持魔法方法（类似 Python dunder），在特定语法上下文中自动调用：

| 魔法方法 | 触发场景 | 签名模式 |
|---------|---------|---------|
| `__get__` | `obj[key]` 索引读取 | `func __get__(self: *T, key: K): V` |
| `__set__` | `obj[key] = value` 索引写入 | `func __set__(self: *T, key: K, value: V): void` |
| `__value__` | 值上下文自动拆箱 | `func __value__(self: *T): U` |
| `__call__` | `obj(args)` 可调用 | `func __call__(self: *T, ...args): R` |
| `__dispose__` | `using` 声明作用域退出时自动调用 | `func __dispose__(self: *T): void` |

```c
type Map = struct {
    data: *Entry
    func __get__(self: *Map, key: string): i32 { return lookup(self.data, key); }
    func __set__(self: *Map, key: string, value: i32): void { insert(self.data, key, value); }
    func __value__(self: *Map): i32 { return size(self.data); }
    func __call__(self: *Map, key: string): i32 { return lookup(self.data, key); }
}

var m = Map{ data: &entries };
var x = m["hello"];       // → __get__(&m, "hello")
m["hello"] = 42;          // → __set__(&m, "hello", 42)
var v: i32 = m;           // → __value__(&m)  自动拆箱
var r = m("hello");       // → __call__(&m, "hello")
```

- `__get__` / `__set__` 拦截 `[]` 索引读写，key 参数为索引值，**不拦截 `.` 属性访问**
- `.` 属性访问始终按字段名直接访问，不做拦截
- `__value__` 类似 JavaScript 的 `Symbol.toPrimitive`，当包装类对象参与运算或需要隐式转换时自动调用，实现自动拆箱
- `__call__` 使结构体实例可像函数一样调用
- `__dispose__` 在 `using` 声明的作用域退出时自动调用，用于资源释放。`using` 声明的类型必须实现 `__dispose__` 方法，且返回类型必须为 `void`
- **不支持运算符重载**，运算符行为固定，`__` 前缀保留给编译器

---

## 7. 自动解引用

Cubec 统一 `.` 和 `->`，指针对象自动解引用：

```c
var pp = .Point{ .x = 1, .y = 2 };
var p = pp.&;
var x = p.x;         // 自动解引用：等价于 p.*.x，无需 p->x
p.x = 10;            // 自动解引用：等价于 p.*.x = 10
var s = p.to_string(); // 自动解引用：self 接收 *Point
```

- **字段访问**：`ptr.field` 自动插入解引用，无需 `->` 语法
- **成员调用**：`ptr.method(args)` 自动解引用后调用，`self` 接收指针
- **赋值**：`ptr.field = value` 自动解引用后写入
- **多级指针**：`pp.*.*.field` 逐级解引用至结构体
- **解引用语法**：`ptr.*`（后缀 `.*`），不是 `*ptr`
- **不存在 `->` 运算符**，统一用 `.`

---

## 8. typeof

`typeof(expr)` 创建新名字指向同一实现 hash，**保留原类型的所有方法和静态字段**：

```c
var p = .Point{ .x= 1, .y= 2 };
type T = typeof(p);     // 新名字，同一实现，保留方法集
T::to_string(&p)        // 等价于 p.to_string()
```

`typeof` 产生 TYPE_TYPE 元类型，可在类型位置使用。既可包裹值表达式（`typeof(42)` → i32），也可包裹类型表达式（`typeof(*i32)` → *i32）。

---

## 9. 类型布局

- **struct** — C 内存布局，字段按声明顺序排列，alignment 对齐，padding 填充
- **union** — 所有字段 offset=0，size=max(fields.size)
- **cunion** — 和 C union 完全一致
- **enum** — 支持显式指定底层类型（`enum Color: u8`），未指定时从第一个 item 推导，后续 item 类型必须一致否则报错
- **packed/align** — 通过 `builtin func packed[T](): T` 和 `builtin func align[N, T](): T` 实现

---

## 10. 检查结果

表达式检查返回 `check_result`，包含左值性：

```c
struct check_result {
    semantic_type_t type;
    bool is_lvalue;        // true = 可赋值
};
```

---

## 11. const/volatile 限定符

TYPE_QUALIFIER 使用 `is_const` + `is_volatile` 双标志表示限定符：

### 11.1 指针 const 映射

| Cubec 语法 | 语义类型 | C 等价 |
|-----------|---------|--------|
| `*const T` | `POINTER(QUALIFIER(const, T))` | `const T*`（指向 const T 的指针） |
| `const *T` | `QUALIFIER(const, POINTER(T))` | `T* const`（const 指针） |
| `*volatile T` | `POINTER(QUALIFIER(volatile, T))` | `volatile T*` |
| `*const volatile T` | `POINTER(QUALIFIER(const\|volatile, T))` | `const volatile T*` |

- `*const T`：指针指向 const 数据，数据不可通过指针修改，但指针可重赋值
- `const *T`：指针本身是 const，不可重赋值，但可通过指针修改数据

### 11.2 const 传播

- **成员访问**：如果 host 类型是 const 限定的（`const Point`），字段类型自动包装为 const
- **指针 auto-deref**：如果指针的 pointee 是 const 限定的（`*const T`），deref 后的字段也是 const
- **解引用**：`const *T` 解引用时先 strip 外层 qualifier 再 deref，结果为 `T`；`*const T` 解引用结果为 `const T`

### 11.3 赋值 const 检查

对 const 限定的 lvalue 赋值报错：

- **标识符**：变量类型是 const → 不可赋值
- **成员访问**：host 类型是 const → 字段不可赋值
- **解引用**：pointee 是 const → 不可通过指针写入

### 11.4 is_mutable 推导

变量声明时 `is_mutable` 根据类型是否 const 设置：

```c
is_mutable = !semantic_type_is_const(var_type);
```

适用于所有声明位置：全局变量、局部变量、函数参数、foreach 迭代变量、闭包 capture 参数。

### 11.5 comptime const 强制

comptime 求值器只强制 const 语义，**忽略 volatile** — volatile 是物理内存读写/多线程同步标识，在虚拟内存+编译期环境中无意义。

### 11.6 工具函数

```c
bool semantic_type_is_const(semantic_type_t type);     // 外层 TYPE_QUALIFIER 且 is_const
bool semantic_type_is_volatile(semantic_type_t type);   // 外层 TYPE_QUALIFIER 且 is_volatile
semantic_type_t semantic_type_strip_qualifier(type);    // 剥离外层 TYPE_QUALIFIER
```

### 11.7 链式限定符解析

解析器循环消费 `const`/`volatile` 关键字，追踪源码出现顺序，保证嵌套正确：

- `const volatile i32` → `const(volatile(i32))`（const 外层）
- `volatile const i32` → `volatile(const(i32))`（volatile 外层）
- `const const i32` → `const(i32)`（重复合并）
