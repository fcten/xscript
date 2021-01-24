package main;

func main() {
    try {
        throw 1;
    } catch (var e int) {
        echo("catch exception 1");
    }

    try {
        test();
    } catch (var e string) {
        echo("catch exception 2");
    }

    try {
        test();
    } catch (var e int) {
        echo("catch exception 3");
    } catch (var e string) {
        echo("catch exception 4");
    } catch (var e bool) {
        echo("catch exception 5");
    }

    throw "uncaught exception";
}

func test() {
    throw "test exception";
}