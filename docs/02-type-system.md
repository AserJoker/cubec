# Cubec 类型系统

> 基于 `include/engine/type.h`、`src/engine/type.c` 及各 `xxx_type.c` 实现。
> 类型系统采用**统一值/类型引擎（VM）**：每个类型是一个带 `vtable_t` 行为分发表的
> 对象，所有类型操作（相等、转换、字段访问、运算、调用等）通过 vtable 分派。

## 1. type_t 与 vtable_t

### 1.1 类型对象

```c
struct _type_t {
  type_kind_t kind;     /* 类型分类（见第 3 节） */
  char *name;           /* 类型名（owned） */
  uint64_t size;        /* 类型大小（字节） */
  uint64_t align;       /* 对齐（字节） */
  bool mut;             /* true = 可变，false = 只读（const） */
  vtable_t vtable;      /* 行为分发表 */
};
```

类型通过 `type_create(allocator, kind, name, size, align, mut, vtable)` 或
`allocator_create(allocator, &g_type_class, &init)` 创建。内置类型在
`vm_create` 时一次性构造并缓存于 `vm_t`。

### 1.2 行为分发表（vtable_t）

类型的行为全部集中在 `vtable_t`，使类型操作（运算、字段访问、调用、转换等）
对编译器透明地分派：

```c
struct vtable_t {
  /* 值层面 */
  value_t (*clone)(vm_t, value_t);
  value_t (*equal)(vm_t, value_t, value_t);
  value_t (*extends)(vm_t, value_t sub, value_t super_val);
  /* 类型层面 */
  value_t (*type_equal)(vm_t, type_t, type_t);
  value_t (*type_extends)(vm_t, type_t, type_t);
  /* 二元 / 一元运算：band/bor/bxor/add/sub/mul/div/mod/shl/shr/gt/lt/
   *                     bnot/lnot/pos/neg */
  /* 隐式转换 */
  value_t (*safe_cast)(vm_t, value_t self, type_t to);
  /* 赋值 */
  value_t (*assignment)(vm_t, value_t lvalue, value_t rvalue);
  /* 字符串表示 */
  value_t (*to_string)(vm_t, value_t self);
  /* 字段访问 obj.field */
  value_t (*get_field)(vm_t, value_t self, const char *name);
  value_t (*set_field)(vm_t, value_t self, const char *name, value_t val);
  /* 下标 obj[index] */
  value_t (*get_item)(vm_t, value_t self, value_t index);
  value_t (*set_item)(vm_t, value_t self, value_t index, value_t val);
  /* 解引用 *ptr */
  value_t (*deref_get)(vm_t, value_t self);
  value_t (*deref_set)(vm_t, value_t self, value_t val);
  /* 切片 value[start..start+count] */
  value_t (*slice)(vm_t, value_t self, uint64_t start, uint64_t count);
  /* 函数调用 */
  value_t (*call)(vm_t, value_t self, size_t argc, value_t *argv);
  /* 成员调用 a.method(args) */
  value_t (*member_call)(vm_t, value_t self, const char *name,
                         size_t argc, value_t *argv);
  /* 静态属性 Type::prop */
  value_t (*get_prop)(vm_t, value_t self, const char *name);
  value_t (*set_prop)(vm_t, value_t self, const char *name, value_t val);
  value_t (*type_get_prop)(vm_t, type_t self, const char *name);
  value_t (*type_set_prop)(vm_t, type_t self, const char *name, value_t val);
  /* union 实例类型检查 value is Type */
  value_t (*is_instance)(vm_t, value_t self, type_t type);
  /* 路径窄化后的原始字段读取（仅 union 实现） */
  value_t (*get_field_raw)(vm_t, value_t self, const char *name);
};
```

> 早期设计的"双层类型架构"（名字层 `type_name_entry` + 实现层 `type_impl`）、
> 结构哈希缓存、独立的 `semantic_type_t` 均**已不存在**。等价能力现由
> `type_t.kind` + `type_t.size/align` + `type_t.vtable` + 各类型各自的实现文件承担。

---

## 2. 结构等价

使用结构等价（鸭子类型），不是名字等价：

- `type A = struct { x: i32 }` 和 `type B = struct { x: i32 }` 大小/对齐/字段一致即视为同一布局
- 类型相等经 `vtable.type_equal` 分派；interface 约束检查经 `vtable.type_extends` 分派
- 结构等价**只看布局与签名**，不计算方法和静态字段

---

## 3. 类型分类（type_kind_t）

实际枚举（`include/engine/type.h`）共 **29 种**，而非早期文档所述的 39 种：

```c
typedef enum type_kind_t {
  TYPE_KIND_TYPE,          /* type 本身（泛型参数 T: type） */
  TYPE_KIND_MODULE,        /* 模块命名空间 */
  TYPE_KIND_CALLABLE,      /* 可调用类型（函数类型） */
  TYPE_KIND_EXCEPTION,     /* 异常类型 */
  /* 基础类型 */
  TYPE_KIND_VOID,
  TYPE_KIND_BOOL,
  TYPE_KIND_I8, TYPE_KIND_I16, TYPE_KIND_I32, TYPE_KIND_I64,
  TYPE_KIND_U8, TYPE_KIND_U16, TYPE_KIND_U32, TYPE_KIND_U64,
  TYPE_KIND_F16, TYPE_KIND_F32, TYPE_KIND_F64,
  TYPE_KIND_CHAR, TYPE_KIND_STR,
  TYPE_KIND_NIL,
  TYPE_KIND_WILDCARD,      /* ? 通配符类型 */
  /* 复合类型 */
  TYPE_KIND_POINTER, TYPE_KIND_ARRAY, TYPE_KIND_SLICE, TYPE_KIND_TUPLE,
  TYPE_KIND_STRUCT, TYPE_KIND_UNION, TYPE_KIND_CUNION,
  TYPE_KIND_ENUM, TYPE_KIND_INTERFACE,
} type_kind_t;
```

> 早期文档中的 `TYPE_QUALIFIER`、`TYPE_GENERIC_INSTANCE`、
> `TYPE_GENERIC_PARAM`、`TYPE_GENERIC_PACK`、`TYPE_GENERIC_VALUE`、
> `TYPE_PACK_INDEX`、`TYPE_OPAQUE`、`TYPE_ERROR`、`TYPE_STRING`（与 `TYPE_STR`
> 区分）等**均已不存在**。const/volatile 现在由 `type_t.mut` 标志表达；
> 泛型相关状态在实例化时由 `vtable` 与 value 层处理，不单独建类型分类。

---

## 4. 隐式类型转换

Cubec 仅允许以下隐式转换（经 `vtable.safe_cast` 分派）：

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

- 全局唯一值 `nil`（由 `vm_get_wildcard_value` 提供通配哨兵，nil 经特殊路径处理）
- 可隐式转换为任意 `*T`
- 任何类型不可转换为 nil
- nil 不可用于定义变量（和 void 一样）
- 可与任意 `*T` 做相等比较

---

## 6. 魔法方法

结构体支持魔法方法（类似 Python dunder），在特定语法上下文中通过 vtable 自动调用：

| 魔法方法 | 触发场景 | 分派到的 vtable |
|---------|---------|---------------|
| `__get__` | `obj[key]` 索引读取 | `get_item` |
| `__set__` | `obj[key] = value` 索引写入 | `set_item` |
| `__value__` | 值上下文自动拆箱 | `safe_cast` / 隐式转换 |
| `__call__` | `obj(args)` 可调用 | `call` |
| `__slice__` | `obj[start:len]` 切片操作 | `slice` |
| `__dispose__` | `using` 声明作用域退出时自动调用 | struct vtable 自定义方法 |

```c
type Map = struct {
    data: *Entry
    func __get__(self: *Map, key: string): i32 { return lookup(self.data, key); }
    func __set__(self: *Map, key: string, value: i32): void { insert(self.data, key, value); }
    func __value__(self: *Map): i32 { return size(self.data); }
    func __call__(self: *Map, key: string): i32 { return lookup(self.data, key); }
}

var m = Map{ data: &entries };
var x = m["hello"];       // → get_item(&m, "hello")
m["hello"] = 42;          // → set_item(&m, "hello", 42)
var v: i32 = m;           // → __value__(&m) 自动拆箱
var r = m("hello");       // → call(&m, "hello")
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
var x = p.x;         // 自动解引用
p.x = 10;            // 自动解引用
var s = p.to_string(); // 自动解引用：self 接收 *Point
```

- **字段访问**：`ptr.field` 经 `value_get_field` 自动插入解引用
- **成员调用**：`ptr.method(args)` 经 `value_member_call` 自动解引用后调用，`self` 接收指针
- **赋值**：`ptr.field = value` 自动解引用后写入
- **解引用语法**：`ptr.*`（后缀 `.*`），不是 `*ptr`
- **不存在 `->` 运算符**，统一用 `.`

---

## 8. typeof

`typeof(expr)` 经类型层计算创建新名字指向同一类型，**保留原类型的所有方法和静态字段**：

```c
var p = .Point{ .x= 1, .y= 2 };
type T = typeof(p);     // 新名字，同一类型，保留方法集
T::to_string(&p)        // 等价于 p.to_string()
```

`typeof` 产生 `TYPE_KIND_TYPE` 元类型，可在类型位置使用。

---

## 9. 类型布局

- **struct** — C 内存布局，字段按声明顺序排列，alignment 对齐，padding 填充
- **union** — 所有字段 offset=0，size=max(fields.size)（外加 `__type__` 标签）
- **cunion** — 和 C union 完全一致
- **enum** — 支持显式指定底层类型（`enum Color: u8`），未指定时从第一个 item 推导
- **packed/align** — 通过 `builtin func packed[T](): T` 和 `builtin func align[N, T](): T` 实现

---

## 10. 值表示与检查结果

表达式检查不再返回 `check_result{type, is_lvalue}`，而是通过 `value_t` 统一承载：

```c
struct _value_t;          /* opaque */
typedef struct _value_t *value_t;
```

- 类型信息经 `value_get_type(value)` 取得（`type_t`）
- 左值性由值对象本身（如变量、字段、解引用结果）决定，可经地址操作 `value_addrof` / `value_member_addr` 表达
- 字段读取 `value_get_field`、赋值 `value_assignment` 等统一经 vtable 分派

---

## 11. const/volatile 限定符

const/volatile 现在由 `type_t.mut` 标志表达（早期 `TYPE_QUALIFIER` 双标志结构已不存在）：

### 11.1 指针 const 映射

| Cubec 语法 | 含义 | C 等价 |
|-----------|------|--------|
| `*const T` | 指向 const T 的指针 | `const T*`（指向 const T 的指针） |
| `const *T` | const 指针 | `T* const`（const 指针） |
| `*volatile T` | 指向 volatile T 的指针 | `volatile T*` |
| `*const volatile T` | 指向 const volatile T | `const volatile T*` |

- `*const T`：指针指向 const 数据，数据不可通过指针修改，但指针可重赋值
- `const *T`：指针本身是 const，不可重赋值，但可通过指针修改数据

### 11.2 const 传播

- **成员访问**：如果 host 类型是 const 限定的（`const Point`），字段类型自动为 const
- **指针 auto-deref**：如果指针的 pointee 是 const 限定的（`*const T`），deref 后的字段也是 const
- **解引用**：`const *T` 解引用结果仍为 `T`；`*const T` 解引用结果为 const T

### 11.3 赋值 const 检查

对 const 限定的 lvalue 赋值报错（经 `vtable.assignment` 检查 `type_t.mut`）：

- **标识符**：变量类型是 const → 不可赋值
- **成员访问**：host 类型是 const → 字段不可赋值
- **解引用**：pointee 是 const → 不可通过指针写入

### 11.4 mut 推导

变量声明时 `mut` 根据类型是否 const 设置：

```c
mut = !type_is_const(var_type);   /* type_t.mut */
```

适用于所有声明位置：全局变量、局部变量、函数参数、foreach 迭代变量、闭包 capture 参数。

### 11.5 comptime const 强制

comptime 求值器只强制 const 语义，**忽略 volatile** — volatile 是物理内存读写/多线程同步标识，在值层+编译期环境中无意义。

### 11.6 工具函数

```c
bool type_is_mut(type_t type);          /* type->mut */
/* const/volatile 查询在早期双层架构中通过 semantic_type_is_const 等实现，
 * 当前由 type_t.mut 直接表达 */
```

### 11.7 链式限定符解析

解析器循环消费 `const`/`volatile` 关键字，追踪源码出现顺序，保证嵌套正确：

- `const volatile i32` → const 外层
- `volatile const i32` → volatile 外层
- `const const i32` → 重复合并
