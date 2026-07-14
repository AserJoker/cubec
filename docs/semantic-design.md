# Cubec 语义分析设计文档

## 1. 核心原则

### 1.1 错误恢复

语义检查不因单点失败而中止，跳过当前节点继续处理兄弟节点，收集所有错误。

- 失败节点返回 `error_type`（占位），防止级联报错
- 诊断收集到列表，遍历不中断
- 错误去重，避免同一错误重复报告

### 1.2 Rustc 风格诊断

```
error: type mismatch
  --> test.cubec:3:14
   |
 3 | var x: i32 = "hello"
   |              ^^^^^^^ expected i32, found string
```

- severity（error/warning/note）
- 主位置 + 附注位置
- 源码缓存按文件名缓存，按行号提取
- 行号对齐、`|` 标尺、`^` 下划线标注 span

### 1.3 显式分配器

所有内存操作走显式 `allocator_t` 参数（Zig 风格），使同一函数可在编译期（虚拟分配器）和运行期（真实分配器）执行。

| 分配器 | 场景 | 内存 |
|--------|------|------|
| `heap_allocator` | 编译器运行时 | 真实堆内存 |
| `comptime_allocator` | comptime 执行 | 虚拟地址空间 |
| `arena_allocator` | 编译器临时结构 | 真实但可批量释放 |

---

## 2. 编译流程

### 2.1 多遍扫描

因 comptime 表达式存在，AST 需要多遍扫描：

**第一遍：声明收集（建作用域）**

遍历 program AST，只注册名字，不深入 body：

- struct/enum/union/cunion/interface → 注册 TYPE 符号，state=NAME_KNOWN
- func → 注册 FUNCTION 符号，state=NAME_KNOWN
- var → 注册 VARIABLE 符号，state=TDZ
- comptime var → 注册 VARIABLE 符号，state=TDZ
- import → 注册 MODULE 符号，注册模块依赖

**第二遍：按序求值/检查**

逐个处理声明，完成后标记为 Evaluated（离开 TDZ）：

- comptime var → 求值，离开 TDZ
- comptime if/for → 执行，决定生成哪些声明
- 类型定义 → 解析字段，计算布局，state=EVALUATED
- 函数 → 检查签名 + body，state=EVALUATED
- 变量 → 检查初始化器，推导类型，state=EVALUATED

**第三遍：函数体检查**

全局作用域已完全解析，逐个深入函数 body 检查。

**第四遍：泛型实例化**

替换具体类型参数，触发函数体内 comptime 求值。

### 2.2 全局 vs 函数体内 comptime

两个不同的执行阶段：

**全局阶段** — 影响后续声明的可见性：

```c
comptime var N = 10;        // 求值后 N 可用
comptime if (N > 5) {       // 依赖 N
    export var big = true;
}
```

**函数体内阶段** — 全局已定型，只影响当前函数：

```c
func foo[T](x: T) {
    comptime if (T extends Numeric) {   // T 在实例化时确定
        var doubled = x * 2;
    }
}
```

---

## 3. TDZ（时序死区）

### 3.1 符号三阶段

```
不存在 → TDZ（名字已注册，值未就绪）→ Evaluated（值可用）
```

- TDZ 中 → 名字可查找，值不可访问，报错"前向值引用"
- Evaluated → 正常使用

### 3.2 类型 TDZ

类型有两级状态：

```
不存在 → NameKnown（可做 *T / []T）→ DefinitionKnown（可做 var / [N]T）
```

- NameKnown → 类型名可用，用于指针/slice 声明（大小/对齐未知）
- DefinitionKnown → 类型定义完整，可用于变量声明、数组长度等

```c
type Node = struct { next: *Node }   // *Node OK，指针大小固定
var n: Node = ...                     // Error — Node 大小/对齐未知（NameKnown 阶段）
```

### 3.3 undefined

`undefined` 是编译期值，分配内存但标记变量为 TDZ：

```c
var x: i32 = undefined;   // TDZ
x = 42;                   // 离开 TDZ
print(x);                 // OK
```

- `var` 语句在非 builtin/extern 场景下**必须有初始化器**
- `= undefined` 是合法初始化器，但变量处于 TDZ 直到显式赋值
- 消除"可能未赋值"问题，简化控制流分析

---

## 4. 作用域与符号表

### 4.1 作用域种类

```
Program
├── GlobalScope          # 顶层声明，按序求值
├── FunctionScope        # 参数 + 局部变量
│   └── BlockScope       # {} 内的变量
│       └── BlockScope   # 嵌套
├── ForScope             # for init 变量
├── ForeachScope         # foreach 迭代变量
├── ComptimeScope        # comptime {} 块作用域
└── TypeScope            # struct/enum/union/cunion/interface 的成员
    ├── InstanceScope    # . 成员（实例字段、方法）
    └── StaticScope      # :: 静态成员（类型级）
```

### 4.2 作用域结构

```c
enum scope_kind {
    SCOPE_GLOBAL,
    SCOPE_FUNCTION,
    SCOPE_BLOCK,
    SCOPE_FOR,
    SCOPE_FOREACH,
    SCOPE_COMPTIME,
    SCOPE_TYPE_INSTANCE,
    SCOPE_TYPE_STATIC,
};

struct scope {
    struct scope *parent;
    vec_t symbols;           // auto_dispose vec of symbol_t
    enum scope_kind kind;
    location_t location;
};
```

### 4.3 符号种类与状态

```c
enum symbol_kind {
    SYMBOL_VARIABLE,
    SYMBOL_FUNCTION,
    SYMBOL_TYPE,
    SYMBOL_MODULE,
    SYMBOL_FIELD,
    SYMBOL_ENUM_ITEM,
    SYMBOL_GENERIC_PARAM,
};

enum symbol_state {
    SYMBOL_TDZ,             // 名字注册，值未就绪
    SYMBOL_NAME_KNOWN,      // 类型名已知（不完整类型）
    SYMBOL_EVALUATED,       // 完全就绪
};
```

### 4.4 查找规则

- **简单标识符 `x`** — 从内向外逐层查找，找到即停，检查 state
- **`.` 成员访问 `expr.field`** — 解析左侧类型，在 InstanceScope 中查找
- **`::` 命名空间访问 `Type::member`** — 解析左侧类型，在 StaticScope 中查找

### 4.5 self 与方法调用

- `self` 始终是指针（`*StructType`）
- member call 是语法糖：`obj.method(args)` → `typeof(obj)::method(&obj, args)`
- 方法查找**只看当前指针类型的方法表**，不沿退化链搜索

---

## 5. 类型系统

### 5.1 两层类型架构

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

### 5.2 结构等价

使用结构等价（鸭子类型），不是名字等价：

- `type A = struct { x: i32 }` 和 `type B = struct { x: i32 }` 是同一类型
- 结构等价**只看字段**，不计算方法和静态字段
- 用结构哈希缓存加速比较，hash 冲突时递归比较

### 5.3 类型分类

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

### 5.4 隐式类型转换

Cubec 仅允许以下隐式转换：

| 转换 | 方向 | 条件 |
|------|------|------|
| 指针退化 | `*A → *B` | A 首字段结构等价于 B（见第6章） |
| nil 转指针 | `nil → *T` | nil 可转为任意指针类型 |
| 整数拓宽 | `i8 → i16 → i32 → i64` | 只允许向更大同符号类型拓宽 |
| 整数拓宽 | `u8 → u16 → u32 → u64` | 只允许向更大同符号类型拓宽 |
| 浮点拓宽 | `f32 → f64` | 只允许向更大浮点类型拓宽 |

- **不允许**隐式窄化（`i32 → i8` 需显式 builtin cast）
- **不允许**符号交叉（`u32 → i32` 需显式 builtin cast）
- **不允许**整浮互转（`i32 → f64` 需显式 builtin cast）
- **不允许** `[]T → *T` 或 `[N]T → []T`（需显式操作）

### 5.5 nil 类型

- 全局唯一值 `nil`
- 可隐式转换为任意 `*T`
- 任何类型不可转换为 nil
- nil 不可用于定义变量（和 void 一样）
- 可与任意 `*T` 做相等比较

### 5.6 魔法方法

结构体支持魔法方法（类似 Python dunder），在特定语法上下文中自动调用：

| 魔法方法 | 触发场景 | 签名模式 |
|---------|---------|---------|
| `__get__` | `obj.field` member get | `func __get__(self: *T, name: []u8): U` |
| `__set__` | `obj.field = value` member set | `func __set__(self: *T, name: []u8, value: U): void` |
| `__value__` | 值上下文解包 | `func __value__(self: *T): U` |
| `__call__` | `obj(args)` 可调用 | `func __call__(self: *T, ...args): R` |

```c
type Proxy = struct {
    target: *i32
    func __get__(self: *Proxy, name: []u8): i32 { return *self.target; }
    func __set__(self: *Proxy, name: []u8, value: i32): void { *self.target = value; }
    func __value__(self: *Proxy): i32 { return *self.target; }
    func __call__(self: *Proxy, x: i32): i32 { return *self.target + x; }
}

var p = Proxy{ target: &val };
var x = p.field;       // → __get__(&p, "field")
p.field = 42;          // → __set__(&p, "field", 42)
var v: i32 = p;        // → __value__(&p)  解包
var r = p(10);         // → __call__(&p, 10)
```

- `__get__` / `__set__` 拦截所有字段读写，name 参数为字段名
- `__value__` 在隐式类型转换/解包时调用
- `__call__` 使结构体实例可像函数一样调用
- **不支持运算符重载**，运算符行为固定，`__` 前缀保留给编译器

### 5.7 typeof

`typeof(expr)` 创建新名字指向同一实现 hash，**保留原类型的所有方法和静态字段**：

```c
var p = Point{ x: 1, y: 2 };
type T = typeof(p);     // 新名字，同一实现，保留方法集
T::to_string(&p)        // 等价于 p.to_string()
```

### 5.8 类型布局

- **struct** — C 内存布局，字段按声明顺序排列，alignment 对齐，padding 填充
- **union** — 所有字段 offset=0，size=max(fields.size)
- **cunion** — 和 C union 完全一致
- **enum** — 支持显式指定底层类型（`enum Color: u8`），未指定时从第一个 item 推导，后续 item 类型必须一致否则报错
- **packed/align** — 通过 `builtin func packed[T](): T` 和 `builtin func align[N, T](): T` 实现

### 5.9 检查结果

表达式检查返回 `check_result`，包含左值性：

```c
struct check_result {
    semantic_type_t type;
    bool is_lvalue;        // true = 可赋值
};
```

---

## 6. 指针退化

### 6.1 退化规则

如果 A 的首字段类型结构等价于 B，则 `*A` 可隐式退化为 `*B`：

```c
type Base = struct { x: i32 }
type Derived = struct { base: Base, y: i32 }

var d = Derived{ ... };
var b: *Base = &d;     // *Derived → *Base 隐式退化 ✓
```

- 退化**仅限指针**，值不允许退化
- 退化是**类型转换**，退化了就是那个类型，不能回头
- 传递性：`*C → *B → *A`（沿首字段链）
- spread 展开后的结构体也可退化：`*Extended → *Base`（前 N 个字段结构等价即满足）

### 6.2 方法查找与退化

方法查找**不沿退化链搜索**。当前指针类型是什么，就只有什么类型的方法：

```c
var a = A{ ... };
a.getX()       // A 有 getX → 调用 ✓
a.getBaseX()   // A 没有 → Error（不会自动退化找 B 的方法）

var b: *B = &a;
b.getBaseX()   // B 有 → 调用 ✓
b.getX()       // B 没有 → Error
```

### 6.3 字段访问与退化

字段访问**不沿首字段链查找**：

```c
d.x          // Error: Derived 没有 x
d.base.x     // OK: 显式访问
```

### 6.4 向下转换

通过 `builtin func cast[T, K](value: *K): *T` 强制转换：

```c
var pa: *A = cast[*A, *B](pb);
```

纯指针重解释，零开销，安全性由程序员保证。

---

## 7. Union 语义

### 7.1 运行时表示

union 在运行时是 `struct { __type__: size_t, data: cunion }` — 带标签的联合体：

```c
union Result[E, T] { value: T; error: E; }
// 运行时等价于：
// struct { __type__: size_t, data: cunion { value: T, error: E } }
```

- `__type__` 存储当前活跃变体的类型 hash（size_t）
- `data` 是 cunion，所有字段 offset=0，size=max(fields.size)
- `__` 前缀字段保留给编译器，用户不允许定义

### 7.2 union_is 类型检查

`union_is[T, U](u: U): bool` 检查 `u.__type__ == T的hash`：

```c
var result = Result[string, i32]{ value: 42 };
if (union_is[i32](result)) {
    // result 当前持有 i32 变体
    var v = result.value;  // 安全访问
}
```

### 7.3 .? 错误传播

`.?` 操作符检查 union 当前类型是否匹配，不匹配则传播错误：

```c
var value = result.value.?;   // 如果 result 不是 value 变体，传播错误
```

- 鸭子类型驱动：编译器检查 `__type__` 是否匹配
- 在函数中，`.?` 触发错误返回（类似 try-catch 传播）
- 在 comptime 中，`.?` 触发编译错误

### 7.4 union 成员

union 支持与 struct 相同的成员类型：

- 字段：`<name> : <type> ;` — 变体字段，存入 cunion data
- 方法：`func <name> ...` — 实例/静态方法
- 静态字段：`var <name> [: <type>] = <expr> ;`
- 关联类型：`type <name> ... ;`
- spread：`...<expr> ;`
- 嵌套声明

---

## 8. Interface 语义

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

---

## 9. 结构体 spread

`...Type` 语法将另一个结构体的字段 AST 原样展开到当前位置：

```c
type Base = struct { x: i32, y: i32 }
type Extended = struct {
    ...Base          // 展开 x: i32, y: i32
    z: i32
}
```

- spread 是字段复用，展开后 Extended 的字段是平铺的 `{ x: i32, y: i32, z: i32 }`
- 因为 Extended 的前 N 个字段（N=Base 字段数）与 Base 结构等价，`*Extended` 可退化为 `*Base`
- spread 不建立命名类型关系（typeof 不同），但结构等价允许指针退化

---

## 10. 类型推导

### 10.1 规则

- **省略类型注解时才推导**，没有占位符语法
- **返回类型必须显式指定**
- 字面量默认 `i32` / `f64`，用后缀指定其他类型

```c
var x = 42;           // → i32
var y: i32 = 42;      // 显式
var z = 42u8;         // → u8（后缀）
var f = 3.14;         // → f64
var g = 3.14f32;      // → f32（后缀）
```

### 10.2 泛型实例化推导

```c
func identity[T](x: T): T { return x; }
var a = identity(42);        // T = i32，从实参推导
var b = identity[i32](42);   // 显式指定
```

---

## 11. 表达式值使用

### 11.1 禁止丢弃返回值

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

### 11.2 全局变量初始化约束

参考 C 语言规则，全局变量的初始化值必须是**编译期可计算**的常量表达式：

```c
var N: i32 = 42;                  // OK: 字面量
var PTR: *i32 = &global_var;      // OK: 全局变量地址（链接期解析，但属于合法编译期计算）
var FN: func(): void = my_func;   // OK: 函数地址（链接期解析）
var X: i32 = some_func();         // Error: 运行时函数调用
```

**编译期可计算**不要求完全在编译期求值，而是指表达式在语义上是常量：

| 表达式 | 是否合法 | 原因 |
|--------|---------|------|
| 字面量 | ✓ | 编译期已知 |
| 枚举项 | ✓ | 编译期已知 |
| `sizeof` / `alignof` | ✓ | 编译期已知 |
| 全局变量地址 `&x` | ✓ | 链接期解析，语义上是常量 |
| 函数地址 `func_name` | ✓ | 链接期解析，语义上是常量 |
| 偏移计算 `offsetof` | ✓ | 编译期已知 |
| 运行时函数调用 | ✗ | 不可预测 |
| 局部变量引用 | ✗ | 不在全局作用域 |
| `comptime` 函数调用 | ✓ | 编译期求值 |

---

## 12. 控制流分析

### 12.1 TDZ 追踪

var 必须初始化，只有 `= undefined` 的变量需要追踪 TDZ 状态：

```c
var x: i32 = undefined;     // TDZ set: {x}
if (cond) {
    x = 1;
}
// TDZ set: {x}（if 路径合并，x 可能仍 TDZ）
print(x);                   // Error: x 可能未赋值
```

### 12.2 不可达代码

return/break/continue 后的语句发出 warning。

### 12.3 return 完整性

非 void 函数所有路径必须有 return。

### 12.4 break/continue 有效性

只能在循环内使用。

---

## 13. defer

### 13.1 语法

只支持块形式：`defer [|captures|] { }`

- 无捕获时 `||` 可省略：`defer { ... }`
- 有捕获：`defer |x, y| { ... }`

### 13.2 捕获语义

**永远按值捕获**（和匿名函数一致）：

```c
var x = 1;
defer |x| { print(x); }   // 捕获 x 的副本 = 1
x = 2;
// defer 输出 1
```

需要观察修改时，显式传指针：

```c
var y = 1;
var yp = &y;
defer |yp| { print(*yp); }   // 捕获指针副本
y = 2;
// defer 输出 2
```

### 13.3 执行顺序

- return 表达式**先求值**
- defer 按栈**逆序**执行（LIFO）
- defer **不允许报错**

### 13.4 匿名函数空捕获

匿名函数的 `||` 也可省略：`func () { }` 等价于 `func || () { }`

---

## 14. 模块系统

### 14.1 import 解析

```c
import std from "std";
import vec as v from "std/vec";
```

- 解析路径 → 定位 `.cubec` 文件
- 解析该文件 → 生成 AST
- 执行声明收集 → 构建模块符号表
- 只暴露 `export` 标记的符号
- 模块符号必须通过 `模块名::` 访问

### 14.2 循环依赖

使用延迟解析 + 模块缓存（类似 Node.js）：

```c
enum module_state {
    MODULE_PARSING,     // 正在声明收集，可能不完整
    MODULE_PARSED,      // 声明收集完成
    MODULE_CHECKED,     // 类型检查完成
};
```

- 遇到已开始解析但未完成的模块 → 返回当前已收集的 export 符号
- PARSING 状态模块的符号允许名字引用（NameKnown），值引用（TDZ）报错
- 与 TDZ 机制完全一致，不需要额外规则

---

## 15. comptime 求值引擎

### 15.1 能力

comptime 解释器本质是 Cubec 解释器，将 Cubec 视作脚本执行：

- ✓ 可以调用非 comptime 函数
- ✗ 不能调用 extern 函数（副作用/IO）
- ✓ 支持内存分配（专用编译期分配器）
- ✓ build 脚本和 test 块通过 comptime 引擎执行

### 15.2 虚拟指针系统

不用真实内存地址，用 Map 分配虚拟地址：

```c
struct comptime_allocator {
    uint64_t next_addr;
    map_t allocations;           // addr → comptime_value
    map_t ptr_lifetimes;         // addr → 作用域层级
};
```

- 分配：返回虚拟地址
- 解引用：从 Map 查找值
- 释放：标记地址无效
- 作用域结束：标记该作用域的虚拟地址为失效

### 15.3 安全检查

| 限制 | 值 | 说明 |
|------|-----|------|
| 最大循环次数 | 1024 | 防止无限循环 |
| 最大调用栈深度 | 256 | 防止无限递归 |
| 最大内存分配 | 可配置 | 防止内存爆炸 |
| extern 函数 | 禁止 | 不可预测的副作用 |
| 指针越界 | 检查 | 虚拟地址空间验证 |
| 悬垂指针 | 检查 | 生命周期追踪 |

### 15.4 编译期值表示

```c
enum comptime_value_kind {
    COMPTIME_VALUE_INT,
    COMPTIME_VALUE_FLOAT,
    COMPTIME_VALUE_BOOL,
    COMPTIME_VALUE_STRING,
    COMPTIME_VALUE_TYPE,
    COMPTIME_VALUE_NIL,
    COMPTIME_VALUE_COMPOSITE,
};
```

### 15.5 执行模型

| 场景 | 执行时机 | 环境 |
|------|---------|------|
| comptime {} | 编译期 | 虚拟内存 + 解释器 |
| comptime var/func | 编译期 | 虚拟内存 + 解释器 |
| test "name" {} | 测试期 | 虚拟内存 + 解释器 |
| build 脚本 | 构建期 | 虚拟内存 + 解释器 |
| 普通函数 | 运行期 | 真实内存 + 编译后代码 |

### 15.6 test 块

`test` 块是编译期执行的测试单元，由 comptime 引擎驱动：

```c
test "addition" {
    var result = 1 + 2;
    assert(result == 3);
}

test "string concat" {
    var s = "hello" + " world";
    assert(s == "hello world");
}
```

- 语法：`test "<name>" { <body> }`
- 在测试期通过 comptime 解释器执行
- 测试失败通过 `assert` 或 `compile_error` 报告（这些函数声明为 `builtin func`）
- test 块可访问当前模块的所有符号（包括非 export）
- test 块不是程序的一部分，不参与最终编译产物

### 15.7 build 脚本

build 脚本通过 comptime 引擎在构建期执行，具体语法和 API 待设计。

---

## 16. 泛型实例化

### 16.1 单态化（Monomorphization）

泛型不是运行时多态，是编译期单态化：

```
identity[i32] → 生成一份 i32 版本的函数体
identity[f64] → 生成一份 f64 版本的函数体
```

### 16.2 约束检查（extends）

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

### 16.3 实例化缓存

用 `(func_name, type_arg_hashes)` 作为缓存 key，同一组具体类型参数只实例化一次。

---

## 17. 装饰器

### 17.1 总览

| 装饰目标 | 签名 | 返回值 | 作用范围 |
|---------|------|--------|---------|
| var | `func [T](init: T): T` | 变换后的值 | 全局变量、结构体静态变量 |
| func | `func [R, ...Args](fn: func(...Args): R): func(...Args): R` | 变换后的函数 | 任意函数，泛型函数在实例化时调用 |
| type | `func [T](): void` | 无，原地编辑 | struct/enum/union 等类型 |
| field | `func [S, T](name: []u8): void` | 无，原地编辑 | 结构体字段（S=结构体类型，T=字段类型） |

### 17.2 var 装饰器

值变换函数，接收初始值，返回变换后的值：

```c
comptime func logged[T](init: T): T {
    print("var initialized: " + tostring(init));
    return init;
}

comptime func positive(init: i32): i32 {
    comptime if (init < 0) {
        compile_error("value must be non-negative");
    }
    return init;
}
```

- 仅用于全局变量和结构体静态变量（编译期可知初始值）
- 泛型版本可装饰任意类型，非泛型只匹配特定类型

### 17.3 func 装饰器

函数变换函数，接收原函数返回同签名新函数：

```c
comptime func traced[R, ...Args](fn: func(...Args): R): func(...Args): R {
    return func |fn| (...args: ...Args): R {
        print("enter: " + fn.name);
        var result = fn(...args);
        print("exit: " + tostring(result));
        return result;
    };
}
```

- 可用于任意位置（局部函数编译后提升到顶层）
- 泛型函数在实例化时调用装饰器

### 17.4 type 装饰器

类型编辑函数，通过 builtin func 原地编辑类型：

```c
comptime func serializable[T](): void {
    add_method[T]("to_string", func (self: *T): string { ... });
}
```

### 17.5 field 装饰器

字段编辑函数，感知结构体类型和字段类型：

```c
comptime func serialized[S, T](name: []u8): void {
    // S = 所属结构体类型
    // T = 字段类型
    // name = 字段名
}
```

### 17.6 求值时机

装饰器在声明收集之后、类型检查之前求值，结果参与后续类型检查。

---

## 18. builtin 内建修饰符

`builtin` 是声明修饰符，标记声明由编译器提供实现，用户不可提供定义：

```c
builtin type RemoveConst[T]    // 内建类型变换
builtin var VERSION: const str // 内建编译期常量
builtin func panic(): void     // 内建函数
builtin func cast[T, K](value: *K): *T       // 指针强制转换
builtin func packed[T](): T                  // packed 类型变换
builtin func align[N, T](): T                // align 类型变换
```

规则：
- `builtin` 与 `extern` 互斥
- `builtin` 与 `export` 正交
- `builtin` 声明无 body/初始值
- `builtin` 可修饰 `type`、`var`、`func` 三种声明

### 18.1 extern

`extern` 标记声明由外部链接提供实现，用于 FFI：

```c
extern func malloc(size: u64): *void
extern func free(ptr: *void): void
extern var errno: i32
```

- `extern` 声明**无 body/初始值**，由链接器解析符号
- `extern` 与 `builtin` 互斥
- `extern` 函数不可在 comptime 中调用（不可预测的副作用/IO）
- `extern` 变量属于全局，地址在链接期解析

---

## 19. checker 模块结构

```
src/engine/
├── diagnostic.c/h        # 诊断系统
├── source.c/h            # 源码缓存
├── scope.c/h             # 作用域栈
├── symbol.c/h            # 符号表示
├── semantic_type.c/h     # 语义类型（两层：name + impl）
├── type_hash.c/h         # 结构哈希计算 + 实现层缓存
├── resolver.c/h          # 名称解析
├── checker.c/h           # 主检查器
├── inference.c/h         # 类型推导
├── flow.c/h              # 控制流分析（TDZ 追踪 + 不可达代码）
├── comptime_eval.c/h     # comptime 解释器
├── comptime_alloc.c/h    # 编译期虚拟分配器
├── module_cache.c/h      # 模块缓存
└── builtin.c/h           # builtin 函数实现
```

### checker 主结构

```c
struct checker {
    allocator_t allocator;
    scope_t *global_scope;
    scope_t *current_scope;
    module_cache_t module_cache;
    diagnostic_list_t diagnostics;
    source_cache_t sources;
    type_impl_cache_t type_cache;
    type_name_table_t type_names;
    comptime_eval_t comptime_eval;
    comptime_allocator_t comptime_alloc;
    flow_state_t flow_state;
    int error_count;
    semantic_type_t error_type;
};
```
