package main;

func test1() bool {
    echo("test false");
    return false;
}

func test2() bool {
    echo("test true");
    return true;
}

func main() {
    // true
    echo(true || test2() || test1());

    // test true
    // true
    echo(false || test2() || test1());

    // test false
    // test true
    // true
    echo(test1() || test2() || test1());

    // test false
    // test false
    // test false
    // false
    echo(test1() || test1() || test1());

    // test true
    // test false
    // false
    echo(true && test2() && test1());

    // test false
    // false
    echo(test1() && test2() && test1());

    // test true
    // test true
    // test false
    // false
    echo(test2() && test2() && test1());
}
