# Cubec 控制流

## 1. TDZ 追踪

var 必须初始化，只有 `= undefined` 的变量需要追踪 TDZ 状态：

```c
var x: i32 = undefined;     // TDZ set: {x}
if (cond) {
    x = 1;
}
// TDZ set: {x}（if 路径合并，x 可能仍 TDZ）
print(x);                   // Error: x 可能未赋值
```

---

## 2. 不可达代码

return/break/continue 后的语句发出 warning。

---

## 3. return 完整性

非 void 函数所有路径必须有 return。

---

## 4. break/continue 有效性

只能在循环内使用。

---

## 5. defer

### 5.1 语法

只支持块形式：`defer [|captures|] { }`

- 无捕获时 `||` 可省略：`defer { ... }`
- 有捕获：`defer |x, y| { ... }`

### 5.2 捕获语义

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
var yp = y.&;
defer |yp| { print(yp.*); }   // 捕获指针副本
y = 2;
// defer 输出 2
```

### 5.3 执行顺序

- return 表达式**先求值**
- defer 按栈**逆序**执行（LIFO）
- defer **不允许报错**

### 5.4 匿名函数空捕获

匿名函数的 `||` 也可省略：`func () { }` 等价于 `func || () { }`

---

## 6. switch 语句

switch 是**纯语句**，不产生值，不可作为表达式使用。

### 6.1 语法

```c
switch (expr) {
    value1 => { ... }
    value2, value3 => { ... }
    _ => { ... }           // 默认分支
}
```

- 每个 match arm 有 `values`（模式表达式列表）和 `body`（语句块）
- body 是语句块，不是表达式
- `_` 表示默认分支

### 6.2 语义

- 检查条件表达式类型
- 逐个检查 match arm 的值和 body
- 不产生返回值
- comptime 求值时按条件执行匹配的 arm

---

## 7. foreach 迭代器协议

### 7.1 语法

```c
foreach (item of collection) {
    // item 是每次迭代的值
}
```

- 使用 `of` 关键字连接迭代变量和迭代器表达式
- 循环变量在 foreach 作用域内有效

### 7.2 迭代器协议

foreach **仅支持迭代器协议**，不直接遍历 slice/array/string。

迭代器必须实现 `next()` 方法，返回包含 `value` 和 `done` 字段的结构体：

```c
type IteratorResult[T] = struct {
    value: T;
    done: bool;
}

type IntIterator = struct {
    data: []i32;
    index: i64;
    func next(self: *IntIterator): IteratorResult[i32] {
        if (self.index >= self.data.len) {
            return .{ .value = 0, .done = true };
        }
        var val = self.data[self.index];
        self.index = self.index + 1;
        return .{ .value = val, .done = false };
    }
}
```

- `next()` 返回 `{value: T, done: bool}`
- `done == true` 表示迭代结束
- `done == false` 时 `value` 为当前迭代值

### 7.3 标准库迭代器

slice/array/string 不能直接用于 foreach，需要通过标准库提供的接口生成临时迭代器：

```c
foreach (item of slice.iter()) { ... }
foreach (ch in "hello".chars()) { ... }
foreach (elem of array.iter()) { ... }
```

> 具体标准库 API 名称待设计。

### 7.4 循环变量 is_mutable

循环变量的可变性根据元素类型推导：

```c
is_mutable = !semantic_type_is_const(element_type)
```

如果迭代器返回的 value 类型是 const，循环变量不可修改。
