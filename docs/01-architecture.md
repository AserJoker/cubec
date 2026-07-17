# Cubec 核心架构

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
- member call 是语法糖：`obj.method(args)` → `typeof(obj)::method(&obj, args)`（详见 `04-expressions.md`）
- 方法查找**只看当前指针类型的方法表**，不沿退化链搜索
