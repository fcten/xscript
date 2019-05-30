function main() {
    try {
        throw 1;
    } catch (auto e) {
        echo("catch exception 1");
    }

    try {
        test();
    } catch (e interface{}) {
        echo("catch exception 2");
    }

    try {
        test();
    } catch (e string) {
        echo("catch exception 3");
    } catch (e int) {
        echo("catch exception 4");
    } catch (e interface{}) {
        echo("catch exception 5");
    }

    throw {errno: 2, errmsg: "uncaught exception"};
}

function test() {
    throw {errno: 1, errmsg: "exception"};
}