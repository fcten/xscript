/* EXPECT
3
*/

var a = 1;

function add(b int) {
    return a + b;
}

function main() {
    echo(add(2));
}
