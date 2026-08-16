# Cubec 指针语义

## 1. 退化规则

如果 A 的首字段类型结构等价于 B，则 `*A` 可隐式退化为 `*B`：

```c
type Base = struct { x: i32 }
type Derived = struct { base: Base, y: i32 }

var d = .Derived{ ... };
var b: *Base = &d;     // *Derived → *Base 隐式退化 ✓
```

- 退化**仅限指针**，值不允许退化
- 退化是**类型转换**，退化了就是那个类型，不能回头
- 传递性：`*C → *B → *A`（沿首字段链）
- spread 展开后的结构体也可退化：`*Extended → *Base`（前 N 个字段结构等价即满足）

---

## 2. 方法查找与退化

方法查找**不沿退化链搜索**。当前指针类型是什么，就只有什么类型的方法：

```c
var a = .A{ ... };
a.getX()       // A 有 getX → 调用 ✓
a.getBaseX()   // A 没有 → Error（不会自动退化找 B 的方法）

var b: *B = &a;
b.getBaseX()   // B 有 → 调用 ✓
b.getX()       // B 没有 → Error
```

---

## 3. 字段访问与退化

字段访问**不沿首字段链查找**：

```c
d.x          // Error: Derived 没有 x
d.base.x     // OK: 显式访问
```

---

## 4. 向下转换

通过 `builtin func cast[T, K](value: *K): *T` 强制转换：

```c
var pa: *A = cast[*A, *B](pb);
```

纯指针重解释，零开销，安全性由程序员保证。

---

## 5. const 指针语义

Cubec 的指针 const 语义与 C 一致（详见 `02-type-system.md` 第11节），但语法不同：

### 5.1 语法到语义映射

| Cubec | 语义类型 | 含义 | C 等价 |
|-------|---------|------|--------|
| `*const T` | `POINTER(QUALIFIER(const, T))` | 指向 const T 的指针 | `const T*` |
| `const *T` | `QUALIFIER(const, POINTER(T))` | const 指针 | `T* const` |
| `*volatile T` | `POINTER(QUALIFIER(volatile, T))` | 指向 volatile T 的指针 | `volatile T*` |
| `*const volatile T` | `POINTER(QUALIFIER(const\|volatile, T))` | 指向 const volatile T | `const volatile T*` |

### 5.2 关键语义规则

- `*const T`：`ptr.* = value` 报错（pointee 是 const），`ptr = &other` 合法（指针本身非 const）
- `const *T`：`ptr = &other` 报错（指针是 const），`ptr.* = value` 合法（pointee 非 const）
- 解引用时，先处理 const/volatile 标志（由 `type_t.mut` 表达），再判断是否为 POINTER 类型
- 成员访问时，const 传播到字段类型（如果 host 是 const，字段也是 const）

---

## 6. spread 与退化

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

> `...` 展开运算符还有其他用途，详见 `04-expressions.md` 第6节。
