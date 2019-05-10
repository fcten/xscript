/* EXPECT
3
*/

function main() {
    var a = 1;
    var b, c;

    do {
        var a = 2;
        b = a;
    } while (false);

    c = a;

    print(b + c);
}
