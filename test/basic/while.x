/* EXPECT
5050
*/

function main() {
    var a = 100000, b = 0;

    while (a > 0) {
        b = b + a;
        a = a - 1;
    }

    print(b);
}
