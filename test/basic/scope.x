/* EXPECT
3
*/

function main() {
    var a = 1;
    var b int, c int;

    do {
        var a = 2;
        b = a;
    } while (false);

    c = a;

    echo(b + c);
}
