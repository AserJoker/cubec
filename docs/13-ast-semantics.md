# Cubec AST 节点语义与 Checker 模块结构

## 1. checker 模块结构

### 1.1 文件职责

```
src/engine/
├── diagnostic.c/h        # 诊断系统
├── source.c/h            # 源码缓存
├── scope.c/h             # 作用域栈
├── symbol.c/h            # 符号表示
├── semantic_type.c/h     # 语义类型（两层：name + impl）
├── type_hash.c/h         # 结构哈希计算 + 实现层缓存
├── resolver.c/h          # 名称解析
├── resolver_types.c/h    # 类型解析
├── checker.c/h           # 主检查器
├── inference.c/h         # 类型推导
├── flow.c/h              # 控制流分析（TDZ 追踪 + 不可达代码）
├── comptime_eval.c/h     # comptime 解释器
├── comptime_alloc.c/h    # 编译期虚拟分配器
├── module_cache.c/h      # 模块缓存
└── builtin.c/h           # builtin 函数实现
```

### 1.2 checker 主结构

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

---

## 2. AST 节点语义确认

### 2.1 逗号表达式（EXPRESSION_COMMA）

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

### 2.2 取地址运算符（EXPRESSION_ADDR）

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

### 2.3 成员调用 desugaring

实例方法调用通过 desugaring 转换为类型级调用：

```c
a.method(arg1, arg2)
→ typeof(a)::method(&a, arg1, arg2)
```

- 指针 auto-deref：如果 `a` 是指针类型且无直接方法，自动解引用后查找方法
- `self` 接收规则：方法第一个参数 `self` 对应 `&a` 的类型
- 详见 `04-expressions.md`

### 2.4 匿名函数与闭包

```c
func |capture_list| [generic_params] (params) [: return_type] { body }
```

- 捕获列表：标识符 only，无表达式初始化
- 运行时：按值捕获
- comptime：按引用捕获
- 空捕获列表：`||`（lexer 将 `||` tokenize 为单 token，需特殊处理）
- 详见 `04-expressions.md`

### 2.5 展开运算符 `...`

三种用途：

1. **类型复用**：struct/union/cunion 中的 `...Type` 展开字段和方法（无类型关系，纯复用）
2. **泛型参数包**：`...T` 定义变参泛型（类似 C++ variadic templates）
3. **调用/初始化展开**：函数调用 `fn(...args)`、初始化列表 `Type{...base}`（编译期确定数量和字段）

详见 `04-expressions.md` 和 `08-generics.md`。

### 2.6 const/volatile 限定符

- 合并为 `expression_type_qualifier` 节点，含 `is_const` + `is_volatile` 双标志
- 指针 const 映射：`*const T` → `POINTER(QUALIFIER(const, T))`，`const *T` → `QUALIFIER(const, POINTER(T))`
- Const 传播：成员访问、解引用、赋值的 const 检查
- `is_mutable = !semantic_type_is_const(var_type)`
- comptime 只强制 const，忽略 volatile
- 链式限定符解析保留源码顺序
- 详见 `02-type-system.md` 和 `03-pointer-semantics.md`

### 2.7 魔法方法

| 方法 | 用途 | 触发场景 |
|------|------|---------|
| `__get__` | 下标读取 `a[i]` | `a[i]` 读取 |
| `__set__` | 下标写入 `a[i] = v` | `a[i]` 赋值 |
| `__value__` | 自动拆箱/隐式转换 | 包装类参与运算或隐式转换 |
| `__call__` | 函数调用 `a()` | 对象作为函数调用 |

- `__get__`/`__set__` 拦截 `[]`，不拦截 `.`
- `__value__` 类似 JS 的 `Symbol.toPrimitive`，用于包装类型的自动拆箱和隐式转换
- 详见 `02-type-system.md`
