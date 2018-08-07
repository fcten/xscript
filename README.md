# xscript

Yet another scripting language.

# Hello World

```
echo "Hello World";
```

# 基本语法

## 注释

```
// 单行注释

/*
 多行注释
*/
```

## 标识符

标示符由字母 (A-Za-z)、数字 (0-9) 和下划线 (_) 所组成，并且不能以数字开头。标识符区分大小写。

## 保留字

类型声明

- auto
- int
- float
- bool
- string
- array

布尔值

- true
- false

函数

- function
- return

流程控制

- if
- else
- do
- while
- break
- continue

# 基本数据类型

## undefined

## integer

64 位有符号整数。

### 整数字面量

十进制：999

二进制：0b010

十六进制：0xFF

## float

双精度浮点数。

### 浮点数字面量

小数表示：123.456

## boolean

布尔类型，值为真 (true) 或假 (false)。

## reference

## string



### 字符串字面量

"this is a string"

## array

下标访问：arr[1]

### 数组字面量

[1, 2, 3]

## function

### 函数字面量

# 运算符

## 算数运算符

- 加 (+)
- 减 (-)
- 乘 (*)
- 除 (/)
- 负号 (-)

## 逻辑运算符

- 与 (&&)
- 或 (||)
- 非 (!)

### 短路运算

## 关系运算符

- 相等 (==)
- 不等 (!=)
- 大于 (>)
- 大于等于 (>=)
- 小于 (<)
- 小于等于 (<=)

## 位运算符

- 按位与 (&)
- 按位或 (|)
- 按位取反 (~)
- 按位异或 (^)
- 左移 (<<)
- 右移 (>>)

## 其它运算符

## 运算符优先级

## 类型转换

# 变量

## 变量声明

## 全局变量

## 局部变量

## 变量作用域

# 函数

## 函数声明

## 函数作用域

# 语句 (statement)

## 表达式语句

```
expression;
```

以下是几种典型的表达式语句

```
a = 1 + 1;

do_something();
```

## if 语句

```
if (condition) {
    // do something
}

if (condition) {
    // do something
} else {
    // do something
}

// TODO else if 尚未实现
if (condition) {
    // do something
} else if (condition) {
    // do something
}
```

注意：语句中的圆括号与花括号均不能省略

## while 语句

```
while (condition) {
    // do something
}
```

注意：语句中的圆括号与花括号均不能省略

## do while 语句

```
do {
    // do something
} while (condition);
```

注意：语句中的圆括号与花括号均不能省略

## for 语句

# 面向对象

# 异常

# 包与模块

## package

## import & export

# 垃圾回收

xscript 使用引用计数为主，标记清除为辅的垃圾回收策略。

- 为了允许同时运行成千上万个虚拟机，xscript 需要尽量节省内存
- 为了实现虚拟机级别的用户隔离，xscript 需要尽量避免 GC 停顿，防止用户遭遇间歇性卡顿
- xscript 需要使用引用计数实现写时复制，减少值传递的开销，以实现默认的值传递策略

# 调试

# 标准库

## IO

## Network

## 协程、线程与进程

## 缓存