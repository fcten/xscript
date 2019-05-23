/* EXPECT
5050
*/

import "io";

function main() {
    var a = 100000, b = 0;

    while (a > 0) {
        b = b + a;
        a = a - 1;
    }

    io.print(b);
}
