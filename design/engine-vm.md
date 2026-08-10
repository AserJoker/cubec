# Engine VM 设计规格

> 基准提交: `8e4a491` | 状态: 设计阶段 | 测试: 1249 passing
>
> 原则: 先整体设计再细节设计，逐个功能点实现落地

---

## 1. Engine = 纯脚本执行 VM，与编译无关

Engine 不是编译器的一部分，而是 cubec 脚本语言的独立运行时：

- 类型系统运行时表示
- 值的存储与操作
- 函数调用与执行
- 作用域变量查找

编译器依赖 engine，engine 不依赖编译器。

---

## 2. 万物皆对象 (Uniform Object Model)

type、module、function 统一为 `value_t`，通过 `kind` 区分：

- 统一分配/释放生命周期
- `name_t.ref` 统一指向 `value_t`
- 对象可作为值传递（type 是一等公民）
- 属性访问 (.field) 统一适用于所有对象
- 无独立 def_t/function_t/namespace_t/stype_t

---

## 3. 单态化在编译层，但依赖 engine 的值操作能力

单态化（泛型实例化）不是 engine 的职责，而是编译器的一个 pass：

- 泛型参数可以是值（如 `Array(T, N: u32)`），不只是类型
- 实例去重需要比较值是否相等（engine 提供值相等语义）
- 实例化时需要 engine 创建具体的类型对象和值对象

架构关系：**编译器依赖 engine**，engine 不依赖编译器。

---

## 4. Shadow Engine — 编译期同步运行，shadow 变量 data=NULL

编译过程中，Shadow Engine 与编译流程同步运行，利用 engine 自身的能力实现：

- 作用域变量查找验证
- 类型合法性检查（赋值兼容、字段存在性等）
- 函数签名匹配

原理：编译期创建的变量/值对象 `data=NULL`（只有类型信息，无实际数据），engine 的类型系统和作用域规则对 shadow 对象和真实对象一视同仁。

- Shadow 对象在编译结束后可被丢弃，或填充 data 变为真实对象
- engine 不需要编译特化逻辑，一套代码服务两种场景

---

## 5. 类型即虚表 — 变量行为由类型对象提供

类型是独立数据结构 `stype_t`（含 name/size/align/vtable），变量持有 `stype_t*` + 数据：

```
engine_add(a, b)           → a.type->vtable.add(a, b)
engine_get_field(obj, "x") → obj.type->vtable.get_field(obj, "x")
```

- `stype_t` 独立于 value 存在，vtable 是其内嵌字段
- 每种类型注册自己的 stype_t 实例（含 vtable）
- engine 核心不硬编码任何类型行为，全部通过 vtable 分发
- 用户自定义类型可注册自定义 vtable（运算符重载）
- duck-typing 自然实现：只要 vtable 匹配就能操作
- 添加新类型 = 注册 vtable，零 engine 核心改动

**Shadow 模式处理：** vtable 方法内部对 data 分支：
- `data=NULL` → 验证/检查分支（Shadow Engine）
- `data!=NULL` → 执行分支（正常执行）
- 一套 vtable，两种模式

---

## 6. 作用域回收 — 域销毁则变量销毁，返回值总是复制

内存管理不用引用计数/GC，而是作用域绑定生命周期：

- 每个作用域持有自己的变量表
- 作用域退出时，该域内所有变量同步销毁
- 函数调用创建新作用域，返回时将返回值**总是复制**到父（调用者）作用域，然后销毁函数作用域
- 无 move 语义优化，依赖 clone vtable 钩子递归深拷贝

---

## 7. own 标记 + SlotMap 虚拟地址

### own 标记

变量有 `own` 标记，区分 data 的生命周期管理：

- `own=true`：变量拥有 data，作用域销毁时释放 data
- `own=false`：变量是引用/借用，data 归其他变量所有，作用域销毁时不释放 data

### 指针 vs 引用

- **指针**：真实数据类型，`sizeof(handle)` 字节，存储**虚拟句柄**（非真实内存地址），类似 C 指针但通过 SlotMap 间接寻址。指针变量 own=true。
- **引用**：非真实数据类型，类似 C++ 引用——编译期别名机制，用于持有对已有数据的引用（如字段引用 `&obj.x` 用于赋值）。引用变量 own=false，data 直接指向目标数据内存位置。

### SlotMap 虚拟寻址

```
真实 C:  pointer → address → pointed-to variable
Engine:  pointer → handle(auto_id + version) → SlotMap → object address → pointed-to object
```

- SlotMap 版本号机制为后续安全检测留空间（版本不匹配 = 已释放）
- 悬垂引用暂不处理

---

## 8. Error 是变量 — 返回值传播，每步检查

- error 是一种变量（error 对象），不是异常/跳转机制
- 函数通过返回值传播 error（类似 Go/Rust Result）
- engine 每执行一条指令都检查返回值是否为 error，若是则立即向外传播
- 调用链上每层检查并传递，直到被 `try`/`catch` 或 `.?`/`.!` 拦截
- engine 无需 try-catch 栈展开机制，error 只是普通数据流
- Shadow Engine 也用同样机制做编译期错误传播

---

## 9. Function 继承自 Callback

- `callback` 是基类型 — 携带签名 + 调用入口
- `function` 继承 callback — 额外携带闭包捕获环境
- function→callback 是**向上转型（类型擦除）**，指针不变，只是通过 callback 视角看不到捕获部分
- `callback` 是用户层面的一等公民：可存储、传递、调用
- `function` 只在引擎内部存在

继承布局：
```
callback { signature, vtable }
function { callback_base, captures[] }
```

向上转型: `(callback*)function_ptr` — 同一指针，callback 视角看不到 captures。

闭包仍然有效：底层 function 对象保留捕获，callback 只是受限接口。

---

## 10. 引擎不感知泛型

- engine 中不存在泛型类型/泛型函数
- 编译器单态化后，向 engine 注册的都是具体实例
- engine 的类型对象、虚表、callback 全部是具体的，无模板参数
- engine 实现更简单，无需处理泛型参数替换

---

## 补充设计

### 类型对象生命周期 = 值生命周期

- 类型与 value 生命周期一致，随作用域回收一起销毁
- 不单独管理类型生命周期（无全局类型注册表）
- 鸭子类型：结构兼容即可，不需要严格 ==
- stype_t vtable 有 `clone` 钩子：clone value 时递归 clone stype_t，防止 value→type 悬垂
- type clone 是递归的（嵌套类型一起 clone）

---

## 架构图

```
┌─────────────────────────────────────────────┐
│              Compiler Pipeline               │
│  lexer → parser → name_collector →          │
│  def_collector → monomorphizer → ...        │
│                    ↓ uses                    │
│            ┌──────────────┐                  │
│            │ Shadow Engine │  (data=NULL)    │
│            └──────┬───────┘                  │
└───────────────────┼─────────────────────────┘
                    │ same code
┌───────────────────┼─────────────────────────┐
│            ┌──────┴───────┐                  │
│            │  Engine VM   │  (data≠NULL)     │
│            │              │                  │
│  value_t ─┤─ stype_t     │  (name/size/align/vtable)                 │
│            ├─ module      │                  │
│            ├─ function    │                  │
│            │   └ callback │                  │
│            ├─ error       │                  │
│            └─ data values │                  │
│               SlotMap ←───┤  pointer targets │
│               scope tree  │  reclamation     │
└─────────────────────────────────────────────┘
```

---

## 数据结构草图

```c
/* VTable — 类型行为分发表（stype_t 的字段） */
typedef struct vtable_t {
  value_t (*add)(value_t a, value_t b);
  value_t (*sub)(value_t a, value_t b);
  value_t (*get_field)(value_t obj, const char *name);
  void     (*set_field)(value_t obj, const char *name, value_t val);
  value_t (*clone)(value_t obj);           /* 深拷贝（递归含 type） */
  void     (*dispose)(value_t obj);          /* 释放 data */
  bool     (*is_error)(value_t obj);         /* 检查是否 error */
  // ... 更多操作
} vtable_t;

/* stype_t — 类型对象（独立数据结构） */
typedef struct stype_t {
  const char *name;      /* 类型名：如 "i32", "MyStruct" */
  uint64_t    size;      /* 字节大小 */
  uint64_t    align;     /* 对齐要求 */
  vtable_t    vtable;    /* 行为分发表（内嵌，非指针） */
} stype_t;

/* Value — 万物皆值 */
typedef struct value_t {
  stype_t *type;         /* 类型对象指针 */
  void    *data;         /* 原始数据缓冲（shadow=NULL） */
  bool     own;          /* 是否拥有 data */
} value_t;

/* SlotMap 条目 — 指针间接寻址 */
typedef struct slot_entry_t {
  uint64_t  version;  /* 释放时递增，检测悬垂 */
  value_t *object;   /* NULL 表示已释放 */
} slot_entry_t;

/* SlotMap — 全局指针目标表 */
typedef struct slotmap_t {
  vec_t entries;      /* vec of slot_entry_t */
  vec_t free_list;    /* 可复用槽位索引 */
  uint64_t next_id;   /* 自增 ID */
} slotmap_t;

/* Scope — 变量表 + 生命周期 */
typedef struct scope_t {
  struct scope_t *parent;
  strmap_t        names;    /* name → value_t* */
  vec_t           children; /* 子作用域 */
} scope_t;
```

---

## 当前状态

- engine 目录只剩 5 对文件：`context`、`module`、`name`、`name_collector`、`scope`
- 所有 `name_t.ref` 为 NULL（phase 1），无 type/value/function/namespace 对象
- 下一步：按功能点逐个实现 engine VM
