/* EXPECT
3
*/

var a = 1;

function add(b int) {
    return a + b;
}

function main() {
    print(add(2));
}
