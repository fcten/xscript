try {
    throw 1
} catch (auto e) {
    echo "catch exception 1"
}

function test() {
    throw Exception{1, "exception"}
}

try {
    test()
} catch (auto e) {
    echo "catch exception 2"
}

try {
    test()
} catch (string e) {
    echo "catch exception 3"
} catch (int e) {
    echo "catch exception 4"
} catch (Exception e) {
    echo "catch exception 5"
}

throw Exception{2, "uncaught exception"}