/* EXPECT
3
*/

var a = 1
var b, c

do {
    var a = 2
    b = a
} while (false)

c = a

echo b + c

