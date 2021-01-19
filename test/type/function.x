package main;

func add(x int, y int) int {
    return x + y;
}

func sub(x int, y int) int {
    return x - y;
}

func main() {
    var f1 func(int, int) int;

    f1 = add;
    echo(f1(1, 2));

    f1 = sub;
    echo(f1(1, 2));

    var f2 = f1;
    echo(f2(1, 2));

    f2 = add;
    echo(f2(1, 2));

    var f3 = func(var x int, var y int) {
        return x * y;
    }
    echo(f3(1, 2));
}