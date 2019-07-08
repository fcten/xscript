/* EXPECT
2
*/

func main() {
    var i = 0;

    do {
        i = i + 1;
    } while (i <= 1);

    echo(i);
}