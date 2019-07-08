func main() {
    try {
        throw 1;
    } catch (e int) {
        echo("catch exception 1");
    }

    try {
        test();
    } catch (e string) {
        echo("catch exception 2");
    }

    try {
        test();
    } catch (e int) {
        echo("catch exception 3");
    } catch (e string) {
        echo("catch exception 4");
    } catch (e bool) {
        echo("catch exception 5");
    }

    throw "uncaught exception";
}

func test() {
    throw "test exception";
}