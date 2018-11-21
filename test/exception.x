try {
    throw 1;
} catch (auto e) {
    echo "catch exception 1\n";
}

function test() {
    throw new Exception("exception object");
}

try {
    test();
} catch (auto e) {
    echo "catch exception 2\n";
}

try {
    test();
} catch (string e) {
    echo "catch exception 3\n";
} catch (int e) {
    echo "catch exception 4\n";
} catch (Exception e) {
    echo "catch exception 5\n";
}

throw new Exception("uncaught exception");