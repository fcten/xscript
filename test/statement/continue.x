/* EXPECT
18
*/

func main() {
    var a = 0, b = 0;

    while (a < 10) {
        a = a + 1;
        if (a < 5) {
            continue;
        }
        b = b + 3;
    }

    echo(b);
}