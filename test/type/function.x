function add(x int, y int) int {
    return x + y;
}

function sub(x int, y int) int {
    return x - y;
}

function main() {
    var f1 function(int, int) int;

    f1 = add;
    echo(f1(1, 2));

    f1 = sub;
    echo(f1(1, 2));

    var f2 = f1;
    echo(f2(1, 2));

    f2 = add;
    echo(f2(1, 2));
}