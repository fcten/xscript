package main;

func test() {
    echo("hello world");
}

func add(var c int, var d int) int {
    var e = 3;
    var f = 4;
    return c + d;
}

func main() {
    test();

    var a = 1;
    var b = 2;
    echo(add(a, b + 1));
}
