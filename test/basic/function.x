func test() {
    echo("hello world");
}

func add(c int, d int) int {
    var e = 3, f = 4;
    return c + d;
}

func main() {
    test();

    var a = 1, b = 2;
    echo(add(a, b + 1));
}
