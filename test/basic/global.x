/* EXPECT
3
5
*/

var a = 1;

function add(b int) int {
    return a + b;
}

function main() {
    echo(add(2));

    a = 3;

    echo(add(2));
}
