function main() {
    try {
        throw 1;
    } catch (auto e) {
        print("catch exception 1");
    }

    try {
        test();
    } catch (e interface{}) {
        print("catch exception 2");
    }

    try {
        test();
    } catch (e string) {
        print("catch exception 3");
    } catch (e int) {
        print("catch exception 4");
    } catch (e interface{}) {
        print("catch exception 5");
    }

    throw {errno: 2, errmsg: "uncaught exception"};
}

function test() {
    throw {errno: 1, errmsg: "exception"};
}