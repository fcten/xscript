# XScript

A scripting language characterized as dynamic, strongly typed, object-oriented and syntactically similar to C.

# Hello World

```
echo "Hello World";
```

# 基本语法 (syntax)

## 注释 (comment)

```
// 单行注释

/*
 多行注释
*/
```

## 标识符 (identifier)

标示符由字母 (A-Za-z)、数字 (0-9) 和下划线 (_) 所组成，并且不能以数字开头。标识符区分大小写。

## 保留字 (reserved word)

类型声明

- auto
- int
- float
- bool
- string
- array

未定义值字面量

- undefined

布尔值

- true
- false

函数

- function
- return

流程控制

- if
- else
- switch
- case
- default
- break
- continue

循环

- do
- while
- for

面向对象

- this
- new
- class
- interface
- extends
- implements
- public
- private
- protected
- final

异常

- try
- catch
- throw

异步

- async
- await

保留字

- delete
- final
- abstract
- finally

# 基本数据类型 (data type)

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

```
string str = "hello";
```

### 字符串字面量

"this is a string"

## array

```
// 创建空数组
array arr = [];

// 追加元素
arr[] = 1;

// 设置指定下标
arr[1] = 2;

// 访问指定下标
echo arr[0];
```

数组下标必须是整数或者字符串类型。

### 数组字面量

```
array arr1 = [1, 2, 3];

// TODO 带下标的数组字面量尚未实现
array arr2 = [
    1: 1,
    "1": 2,
];
```

## function

### 函数字面量

# 运算符 (operator)

## 算数运算符

- 加 (+)
- 减 (-)
- 乘 (*)
- 除 (/)
- 负号 (-)

## 逻辑运算符

- 与 (&&)
- 或 (&#124;&#124;)
- 非 (!)

### 短路运算

与 (&&) 和 或 (&#124;&#124;) 运算符存在短路特性：

```
bool ret1 = fun1() && fun2();

// 等价于

bool ret2;
if (fun1() == false) {
    ret2 = false;
} else {
    ret2 = fun2();
}
```

```
bool ret1 = fun1() || fun2();

// 等价于

bool ret2;
if (fun1() == true) {
    ret2 = true;
} else {
    ret2 = fun2();
}
```

## 关系运算符

- 相等 (==)
- 不等 (!=)
- 大于 (>)
- 大于等于 (>=)
- 小于 (<)
- 小于等于 (<=)

## 位运算符

- 按位与 (&)
- 按位或 (&#124;)
- 按位取反 (~)
- 按位异或 (^)
- 左移 (<<)
- 右移 (>>)

## 赋值运算符

- 赋值 (=)

## 其它运算符

- 括号 (())
- 下标访问 ([])
- 类型获取 (typeof)

## 运算符优先级

1. 括号 (())、下标访问 ([])
2. 成员访问 (->)
3. 非 (!)、按位取反 (~)、负号 (-)、类型获取 (typeof)、对象创建（new）
4. 乘 (*)、除 (/)
5. 加 (+)、减 (-)
6. 左移 (<<)、右移 (>>)
7. 大于 (>)、小于 (<)、大于等于 (>=)、小于等于 (<=)
8. 相等 (==)、不等 (!=)
9. 按位与 (&)
10. 按位异或 (^)
11. 按位或 (&#124;)
12. 与 (&&)
13. 或 (&#124;&#124;)
14. 赋值 (=)

## 运算符结合性

在 xscript 中，以下运算符是右结合的：

- 单目运算符
- 赋值运算符 (=)
- 与运算符 (&&)
- 或运算符 (&#124;&#124;)

其它运算符均为左结合。

```
bool a1 = !!!true;

// 等价于

bool a1 = !(!(!true));
```

```
int a, b, c;

a = b = 1;

// 等价于

a = (b = 1);

// 等价于

c = 1;
b = c;
a = c;
```

```
bool a1 = false && true && false;

// 等价于

bool a1 = false && (true && false);

// 等价于

bool a1 = false;
```

```
int a1 = 1 + 2 + 3;

// 等价于

int a1 = (1 + 2) + 3;
```

## 类型转换

# 变量 (variable)

## 变量声明

变量必须先声明再使用。

```
var_type var_name;

var_type var_name = init_value;
```

以下是一些示例：

```
int a;

a = 1; // OK
b = 1; // Error: undefined variable `b`
```

## 全局变量

## 局部变量

## 变量作用域

xscript 支持函数级变量作用域和块级变量作用域。

```
function add (int a, int b) int {
    int c = 1;
    return a + b + c;
}

if (true) {
    int d = 1;
}

echo a; // Error: undefined variable `a`
echo b; // Error: undefined variable `b`
echo c; // Error: undefined variable `c`
echo d; // Error: undefined variable `d`
```

## 变量值传递

在 xscript 中，变量赋值、参数传递均是通过值传递的方式完成的。

```
array a = [1,2,3];

function assign(array a) {
    a = [4,5,6];
}

assign(a);

echo a; // output [1,2,3]
```

在上面的代码中，调用函数 assign(a) 并不会改变变量 a 的值。

## 引用类型变量

在 xscript 中，string、array、object、function 类型均为隐式引用类型。

```
array a = [1,2,3];

function assign(array a) {
    a[] = 4;
}

assign(a);

echo a; // output [1,2,3,4]
```

在上面的代码中，调用函数 assign(a) 会使变量 a 的值发生改变，因为参数 a 与 变量 a 是对同一个数组的引用。

# 函数 (function)

函数不需要声明，可以先使用再定义。

```
function func_name(arg_type arg_name, ...) ret_type {
    func_body
}
```

返回值类型可以被省略。返回值类型被省略时默认为 auto。

以下是一些示例：

```
echo add(1, add(2, 3));

function add(int a, int b) int {
    return a + b;
}
```

## 可选参数

定义函数时可以设定参数的默认值，设定了默认值的参数可以在函数调用时省略。

```
echo add();

function add(int a = 1, int b = 2) int {
    return a + b;
}
```

注意：因为形参和实参必须一一对应，所以包含默认值的参数应当被置于参数列表的最后。

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
## 流程控制语句

### if 语句

```
if (expression) {
    // do something
}

if (expression) {
    // do something
} else {
    // do something
}

if (expression) {
    // do something
} else if (expression) {
    // do something
} else if (expression) {
    // do something
} else {
    // do something
}
```

if 语句中的 expression 的运算结果必须是 boolean。

如果 expression 的值为 true，控制流会跳转到紧跟的代码块。如果值为 false，控制流会跳转到下一个代码块。

if 语句中的任意代码块执行结束时，控制流会跳出 if 语句。

注意：语句中的圆括号与花括号均不能省略

### switch 语句

```
switch (expression) {
    case constant-expression:
       // do something
       break;
    case constant-expression:
       // do something
       break;
    default:
       // do something
}
```

switch 语句中的 expression 值的类型必须是 integer 或者 string。

switch 语句中可以有任意数量的 case 代码块。当某一个 case 匹配均为 true 时，控制流将跳转到该 case 代码块。

switch 语句中可以有最多一个 default 代码块。当所有的 case 匹配均为 false 时，控制流将跳转到 default 代码块。

case 代码块中的 constant-expression 的值必须能够在编译时确定，其类型必须是 integer 或者 string。同一个 switch 语句中的各个 constant-expression 的值必须各不相同。

当一个 case 代码块或 default 代码块执行完毕时，控制流会继续执行下一个代码块。如果不希望继续执行下一个代码块，应当使用 break 主动跳出 switch 语句。

### break 语句

```
break;
```

break 语句用于跳出循环语句和 switch 语句。

### continue 语句

```
continue;
```

continue 语句用于跳过剩余的循环体，继续开始下一次循环。

## 循环语句

### while 语句

```
while (expression) {
    // do something
}
```

while 语句中的 expression 的运算结果必须是 boolean。

注意：语句中的圆括号与花括号均不能省略

### do-while 语句

```
do {
    // do something
} while (expression);
```

do-while 语句中的 expression 的运算结果必须是 boolean。

注意：语句中的圆括号与花括号均不能省略

### for 语句

```
for (expression; expression; expression) {
    // do something
}
```

第一个 expression 会在循环开始前执行一次。

第二个 expression 会在每次循环开始前执行，并且它的运算结果必须是 boolean。如果它的运算结果为 false，则会跳出循环。

第三个 expression 会在每次循环结束后执行。

所有三个 expression 均可省略，如下代码也是合法的：

```
for (;;) { // endless loop
    // do something
}
```

注意：语句中的圆括号与花括号均不能省略

# 面向对象

## 类 (class)

类必须先定义再使用。

```
class Test {
    public function print() {
        echo "Test:print";
    }
}

Test obj = new Test;
obj->print();
```
## 接口 (interface)

## 封装

## 继承

## 多态

## 构造函数

可以通过定义一个名为 `constructor` 的方法为类添加一个构造函数。

```
class Test {
    private string msg;

    public function constructor(string msg) {
        this->msg = msg;
    }

    public function print() {
        echo this->msg;
    }
}

(new Test("123"))->print();
(new Test("456"))->print();
```

构造函数是一个普通的函数，可以像其他方法一样在任何时候被调用。但是除非必要，否则不建议手动调用构造函数。

每当通过 new 操作符创建新的对象之后，该对象的构造函数会被隐式调用。当构造函数被隐式调用时，该方法的返回值会被忽略。

构造函数不能阻止对象被创建，因为在调用构造函数时对象已经创建完毕了。在构造函数中抛出异常可以阻止初始化失败的对象被赋值给其它变量，未被赋值的对象会在未来的某个时间通过垃圾回收机制销毁。

## 析构函数

# 异常

```
try {
    throw 1;
} catch (auto e) {
    echo "catch exception 1";
}

function test() {
    throw 2;
}

try {
    test();
} catch (auto e) {
    echo "catch exception 2";
}

throw 3; // uncaught exception 3
```

## 抛出异常 (throw)

## 捕获异常 (catch)

# 异步

xscript 使用协程处理异步。

## 协程

## async

## await

# 垃圾回收 (garbage collection)

xscript 默认使用引用计数为主，标记清除为辅的垃圾回收策略。

- 为了允许同时运行成千上万个虚拟机，xscript 需要尽量节省内存
- 为了实现虚拟机级别的用户隔离，xscript 需要尽量避免 GC 停顿，防止用户遭遇间歇性卡顿

TODO: 引用计数机制可以单独关闭，只使用标记清除完成垃圾回收。

# 调试

# 包与模块

## 包 (package)

## import & export

# 标准库 (std)

xscript 标准库是以包的形式提供的。

- std.io.*
- std.time.*
- std.math.*
- std.string.*
- std.array.*
- std.coroutine.*
- std.exception.*

## 扩展库

- os.thread.*
- os.process.*
- redis.*
- mysql.*
- net.http.*
- encoding.json.*
- compress.*
- crypto.*
- image.*
- regexp.*