# Cubec Comptime 求值引擎

## 1. 能力

comptime 解释器本质是 Cubec 解释器，将 Cubec 视作脚本执行：

- ✓ 可以调用非 comptime 函数
- ✗ 不能调用 extern 函数（副作用/IO）
- ✓ 支持内存分配（专用编译期分配器）
- ✓ build 脚本和 test 块通过 comptime 引擎执行

---

## 2. 虚拟指针系统

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

---

## 3. 安全检查

| 限制 | 值 | 说明 |
|------|-----|------|
| 最大循环次数 | 1024 | 防止无限循环 |
| 最大调用栈深度 | 256 | 防止无限递归 |
| 最大内存分配 | 可配置 | 防止内存爆炸 |
| extern 函数 | 禁止 | 不可预测的副作用 |
| 指针越界 | 检查 | 虚拟地址空间验证 |
| 悬垂指针 | 检查 | 生命周期追踪 |

---

## 4. 编译期值表示

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

---

## 5. 执行模型

| 场景 | 执行时机 | 环境 |
|------|---------|------|
| comptime {} | 编译期 | 虚拟内存 + 解释器 |
| comptime var/func | 编译期 | 虚拟内存 + 解释器 |
| test "name" {} | 测试期 | 虚拟内存 + 解释器 |
| build 脚本 | 构建期 | 虚拟内存 + 解释器 |
| 普通函数 | 运行期 | 真实内存 + 编译后代码 |

统一存储模型：env 存储 name → addr 映射，alloc 存储 addr → value 映射。指针使用引用语义。

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

comptime for (var i = 0; i < N; i = i + 1) {
    // 编译期循环生成声明
}

comptime {
    // 编译期执行块
}
```

- `comptime if` — 编译期条件分支，决定生成哪些声明
- `comptime for` — 编译期循环，可批量生成声明
- `comptime { }` — 编译期执行块
- 所有 comptime 语句在第二遍（按序求值/检查）阶段执行
