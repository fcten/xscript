/* EXPECT
3
5
*/

package main;

var a = 1;

func add(b int) int {
    return a + b;
}

func main() {
    echo(add(2));

    a = 3;

    echo(add(2));
}
