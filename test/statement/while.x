/* EXPECT
5050
*/

package main;

import std.io;

func main() {
    var a = 100;
    var b = 0;

    while (a > 0) {
        b = b + a;
        a = a - 1;
    }

    io.print(b);
}
