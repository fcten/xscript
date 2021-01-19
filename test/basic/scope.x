/* EXPECT
3
*/

package main;

func main() {
    var a = 1;
    var b int;
    var c int;

    do {
        var a = 2;
        b = a;
    } while (false);

    c = a;

    echo(b + c);
}
