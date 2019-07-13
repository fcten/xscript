/* EXPECT
101
*/

func main() {
    var a = 0;

    while (true) {
        a = a + 1;
        if (a > 100) {
            break;
        }
    }

    echo(a);
}