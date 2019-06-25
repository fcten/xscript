function add(x int, y int) int {
    return x + y;
}

function sub(x int, y int) int {
    return x - y;
}

function main() {
    var f function(int, int) int;

    f = add;
    echo(f(1, 2));

    f = sub;
    echo(f(1, 2));
}