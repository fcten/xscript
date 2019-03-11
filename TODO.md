xscript 的目标：

1 可读性、可维护性
2 简洁、开发效率
3 鲁棒性
4 性能

热更新

GC 算法结合自行实现的内存管理算法

在可能的情况下，使用真实的数组而不是哈希表以提高性能

支持逗号表达式？
支持 ++？
支持 += -= *= /= 等操作符？
支持匿名函数与闭包？

编译时检查：
禁止有返回值的函数存在没有 return 的分支
禁止没有返回的函数被用于参与运算
禁止使用未初始化的变量
禁止存在未使用的局部变量和全局变量

控制线程 -> 消息队列 -> 死循环防护

重新设计 import & export & require
命名空间？

重新实现常量表

重新设计类

array 的成员默认是 auto 的，增加 array<type> 语法支持？
array -> array<undefined, undefined>
array<string> -> array<int, string>
array<string, string>

支持 for 循环条件中声明变量

支持 struct, typedef