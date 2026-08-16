# Cubec Comptime 求值引擎

## 1. 能力

comptime 解释器本质是 Cubec 解释器，将 Cubec 视作脚本执行：

- ✓ 可以调用非 comptime 函数
- ✗ 不能调用 extern 函数（副作用/IO）
- ✓ 支持内存分配（专用编译期分配器）
- ✓ build 脚本和 test 块通过 comptime 引擎执行

---

## 2. 编译期执行模型

comptime 求值复用统一的 VM 值/类型引擎（见 `15-runtime-execution.md`）。编译期
不分配真实内存，而是通过 VM 的 `value_t` 承载编译期常量：

- 编译期值通过 `value_create` / `value_create_shadow` 创建，存放在作用域的 `values` 向量中
- 指针语义经类型 vtable 的 `deref_get` / `deref_set` 分派
- 作用域退出时，`scope_dispose` 自动释放该作用域的 `values` / `types` / `strings` 等
- 早期文档描述的 `comptime_allocator`（Map 分配虚拟地址）结构已不存在

> 注：编译期能力（调用非 comptime 函数、支持内存分配、test 块、build 脚本）的语义
> 目标不变，但底层实现已统一到 VM 引擎。

---

## 3. 安全检查

> 早期文档列出的具体限制常量（最大循环 1024、最大调用栈 256、最大内存分配可配置）
> 在**当前源码中未找到对应实现**（VM 内无此类硬限制常量）。以下为设计目标的语义约束：

| 限制 | 状态 | 说明 |
|------|------|------|
| 最大循环次数 | 规划中 | 防止无限循环 |
| 最大调用栈深度 | 规划中 | 防止无限递归（VM 有 `call_stack` 向量） |
| 最大内存分配 | 规划中 | 防止内存爆炸 |
| extern 函数 | 语义约束（目标） | 不可预测的副作用，编译期不应调用 |
| 指针越界 / 悬垂指针 | 由 VM 值层保证 | 值对象经 `value_t` 管理，无裸虚拟地址 |

---

## 4. 编译期值表示

> 早期文档中的 `comptime_value_kind` 枚举（`COMPTIME_VALUE_INT` 等）已不存在。
> 编译期值统一用 `value_t` 表示，其具体类别由 `value_get_type(value)->kind`
> （`type_kind_t`，如 `TYPE_KIND_I32` / `TYPE_KIND_STR` / `TYPE_KIND_TYPE` 等）
> 区分，见 `02-type-system.md` 第 3 节。

---

## 5. 执行模型

| 场景 | 执行时机 | 环境 |
|------|---------|------|
| comptime {} | 编译期 | VM 值层（编译期求值） |
| comptime var/func | 编译期 | VM 值层（编译期求值） |
| test "name" {} | 测试期 | VM 值层（编译期求值） |
| build 脚本 | 构建期 | VM 值层（编译期求值） |
| 普通函数 | 运行期 | 真实内存 + C 后端生成代码 |

统一存储模型：作用域的 `names` 表存储 name → `value_t` 映射，`values` 向量存储
实际值对象。指针使用引用语义（经类型 vtable 的 `deref_*` 分派）。

---

## 6. test 块

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

---

## 7. build 脚本

build 脚本通过 comptime 引擎在构建期执行，具体语法和 API 待设计。

当前示例形式（`demo/build.cubec`，仅供参考，非最终方案）：

```c
import build from "build";
import error from "error";
export func build_main(builder: *build::Builder): error::Result[build::BuildError, void] {
  var main_target = builder::new("main", build::Execute);
  main_target.addFlag("-O2");
  main_target.setOutputDirectory(builder.workspace);
  _ = main_target.build().?;
}
```

关键特征：
- 构建功能由**标准库**实现（`build` 模块），不是编译器内置
- build 脚本是普通的 Cubec 源文件，通过 `export func build_main` 作为入口
- 编译器识别 `build.cubec` 文件并在构建期执行
- 可使用完整的 Cubec 语法（import、泛型、.? 错误传播等）

---

## 8. const 强制

comptime 求值器只强制 const 语义，**忽略 volatile**。volatile 是物理内存读写/多线程同步标识，在虚拟内存+编译期环境中无意义。

### 8.1 标识符赋值

检查 `sym->variable.is_mutable`，如果为 false（const 变量），报错 "cannot assign to const variable"。

### 8.2 成员赋值

- 直接复合类型（struct）：检查 host value 的类型是否 const
- 指针 host：检查 pointee 类型是否 const

### 8.3 解引用赋值

检查指针的 pointee 类型是否 const，如果是则报错。

---

## 9. comptime 语句

comptime 既是 var/func 的**修饰符**，也有独立的**语句形式**：

### 9.1 comptime 修饰符

修饰变量和函数声明，使其在编译期求值：

```c
comptime var N = 10;
comptime func factorial(n: i32): i32 { ... }
```

- 与 `using` 互斥（见 `09-modifiers.md`）
- comptime var 在编译期求值后可被后续声明引用
- comptime func 在编译期执行，不生成运行时代码

### 9.2 comptime 语句

独立语句形式，在编译期条件执行：

```c
comptime if (N > 5) {
    export var big = true;
}

comptime foreach (item of items) {
    // 编译期迭代器展开
}

comptime {
    // 编译期执行块
}
```

- `comptime if` — 编译期条件分支，条件必须编译期求值为 bool，未采取分支不做类型检查
- `comptime foreach` — 编译期迭代器展开，使用 `of` 关键字指定迭代器
- `comptime { }` — 编译期执行块
- 所有 comptime 语句在第二遍（按序求值/检查）阶段执行
